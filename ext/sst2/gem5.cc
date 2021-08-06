#include <sst/core/sst_config.h>
#include <sst/core/componentInfo.h>
#include <Python.h>  // Before serialization to prevent spurious warnings

#include "gem5.hh"

// System headers
#include <string>
#include <vector>

// gem5 Headers
#include <sim/core.hh>
#include <sim/init.hh>
#include <sim/init_signals.hh>
#include <sim/system.hh>
#include <sim/sim_events.hh>
#include <sim/sim_object.hh>
#include <base/logging.hh>
#include <base/debug.hh>

#include <cassert>

//#ifdef fatal  // gem5 sets this
//#undef fatal
//#endif

// More SST Headers
#include <core/timeConverter.h>

gem5Component::gem5Component(SST::ComponentId_t id, SST::Params& params):
    SST::Component(id)
{
    output.init("gem5Component-" + getName() + "->", 1, 0, SST::Output::STDOUT);

    // Register a handler to be called on a set frequency.
    SST::TimeConverter *clock = registerClock(
        "1MHz",
        new SST::Clock::Handler<gem5Component>(this, &gem5Component::clockTick)
    );

    // how many gem5 cycles will be simulated within an SST clock tick
    gem5_sim_cycles = clock->getFactor();

    // "cmd" -> gem5's Python
    std::string cmd = params.find<std::string>("cmd", "");
    if (cmd.empty()) {
        output.output(
            CALL_INFO, "Component %s must have a 'cmd' parameter.\n",
            getName().c_str()
        );
        exit(-1);
    }

    // Telling SST the command line call to gem5
    std::vector<char*> args;
    args.push_back(const_cast<char*>("sst.x"));
    splitCommandArgs(cmd, args);
    output.output(CALL_INFO, "Command string:  [sst.x %s]\n", cmd.c_str());
    for (size_t i = 0; i < args.size(); ++i) {
        output.output(CALL_INFO, "  Arg [%02zu] = %s\n", i, args[i]);
    }

    initPython(args.size(), &args[0]);

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}

gem5Component::~gem5Component()
{
}

void
gem5Component::init(unsigned phase)
{
    output.output(CALL_INFO," init phase: %u\n", phase);
    if (phase == 0) {
        const std::vector<std::string> instantiate_command_2 = {
            "m5.instantiate_step_2()"
        };
        this->execPythonCommands(instantiate_command_2);
    }

    /*
    const std::vector<std::string> find_sim_object_commands = {
        "print('-----------------------------------------')",
        "from m5.objects import OutgoingRequestBridge, Root",
        "root = Root.getInstance()",
        "print(root.find_all(OutgoingRequestBridge))",
        "for obj in root.descendants(): print(type(obj))"
    };
    this->execPythonCommands(find_sim_object_commands);
    */
    SSTResponder* system_port = loadUserSubComponent<SSTResponder>("system_port", 0);
    gem5_connectors.push_back(system_port);
    SSTResponder* cache_port = loadUserSubComponent<SSTResponder>("cache_port", 0);
    gem5_connectors.push_back(cache_port);
    //SSTResponder* icache_port = loadUserSubComponent<SSTResponder>("icache_port", 0);
    //gem5_connectors.push_back(icache_port);
    //SSTResponder* dcache_port = loadUserSubComponent<SSTResponder>("dcache_port", 0);
    //gem5_connectors.push_back(dcache_port);
}

void
gem5Component::setup()
{
    output.verbose(CALL_INFO, 1, 0, "Component is being setup.\n");
}

void
gem5Component::finish()
{
    output.verbose(CALL_INFO, 1, 0, "Component is being finished.\n");
}

bool
gem5Component::clockTick(SST::Cycle_t currentCycle)
{
    // what to do in a SST's Tick

    /*
    for (auto requestor: requestors)
        requestor->clock();
    */

//    return true;

    GlobalSimLoopExitEvent *event = simulate(gem5_sim_cycles);
    clocks_processed++;
    if (event != simulate_limit_event) { // gem5 exits due to reasons other than reaching simulation limit
        output.output("exiting: curTick()=%lu cause=`%s` code=%d\n",
            curTick(), event->getCause().c_str(), event->getCode()
        );
        primaryComponentOKToEndSim();
        return true;
    }

    // returning False means the simulation should go on
    return false;

}

#define PyCC(x) (const_cast<char *>(x))

int
gem5Component::execPythonCommands(const std::vector<std::string>& commands)
{
    PyObject *dict = PyModule_GetDict(this->python_main);

    // import the main m5 module
    PyObject *result;

    for (auto const command: commands) {
        result = PyRun_String(command.c_str(), Py_file_input, dict, dict);
        if (!result) {
            PyErr_Print();
            return 1;
        }
        Py_DECREF(result);
    }
}

int
gem5Component::startM5(int argc, char **_argv)
{
#if HAVE_PROTOBUF
    // Verify that the version of the protobuf library that we linked
    // against is compatible with the version of the headers we
    // compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;
#endif


#if PY_MAJOR_VERSION >= 3
    typedef std::unique_ptr<wchar_t[], decltype(&PyMem_RawFree)> WArgUPtr;
    std::vector<WArgUPtr> v_argv;
    std::vector<wchar_t *> vp_argv;
    v_argv.reserve(argc);
    vp_argv.reserve(argc);
    for (int i = 0; i < argc; i++) {
        v_argv.emplace_back(Py_DecodeLocale(_argv[i], NULL), &PyMem_RawFree);
        vp_argv.emplace_back(v_argv.back().get());
    }

    wchar_t **argv = vp_argv.data();
#else
    char **argv = _argv;
#endif

    PySys_SetArgv(argc, argv);

    // We have to set things up in the special __main__ module
    //PyObject *module = PyImport_AddModule(PyCC("__main__"));
    python_main = PyImport_AddModule(PyCC("__main__"));
    if (python_main == NULL)
        panic("Could not import __main__");

    const std::vector<std::string> commands = {
        "import m5",
        "m5.main()"
    };
    this->execPythonCommands(commands);

#if HAVE_PROTOBUF
    google::protobuf::ShutdownProtobufLibrary();
#endif

    return 0;
}

void
gem5Component::initPython(int argc, char *_argv[])
{
    // should be similar to gem5's src/sim/main.cc
    PyObject *mainModule, *mainDict;

    int ret;

    // Initialize m5 special signal handling.
    initSignals();

#if PY_MAJOR_VERSION >= 3
    std::unique_ptr<wchar_t[], decltype(&PyMem_RawFree)> program(
        Py_DecodeLocale(_argv[0], NULL),
        &PyMem_RawFree);
    Py_SetProgramName(program.get());
#else
    Py_SetProgramName(_argv[0]);
#endif

    // Register native modules with Python's init system before
    // initializing the interpreter.
    if (!Py_IsInitialized()) {
        registerNativeModules();
        // initialize embedded Python interpreter
        Py_Initialize();
    }
    else {
        // https://stackoverflow.com/a/28349174
        PyImport_AddModule("_m5");
        PyObject* module = EmbeddedPyBind::initAll();
        PyObject* sys_modules = PyImport_GetModuleDict();
        PyDict_SetItemString(sys_modules, "_m5", module);
        Py_DECREF(module);
    }


    // Initialize the embedded m5 python library
    ret = EmbeddedPython::initAll();

    if (ret == 0)
    {
        // start m5
        this->startM5(argc, _argv);
    }
    else
    {
        output.output(CALL_INFO, "Not calling m5Main due to ret=%d\n", ret);
    }

    const std::vector<std::string> instantiate_command_1 = {
        "m5.instantiate_step_1()"
    };
    this->execPythonCommands(instantiate_command_1);
}

void
gem5Component::splitCommandArgs(std::string &cmd, std::vector<char*> &args)
{
    const std::array<char, 4> delimiters = { {'\\', ' ', '\'', '\"'} };

    std::vector<std::string> parsed_args1;
    std::vector<std::string> parsed_args2;

    auto prev = &(parsed_args1);
    auto curr = &(parsed_args2);

    curr->push_back(cmd);

    for (auto delimiter: delimiters)
    {
        std::swap(prev, curr);
        curr->clear();

        for (auto part: *prev)
        {
            size_t left = 0;
            size_t right = 0;
            size_t part_length = part.size();

            while (left < part_length)
            {
                while ((left < part_length) && (part[left] == delimiter))
                    left++;
                
                if (!(left < part_length))
                    break;

                right = part.find(delimiter, left);
                if (right == part.npos)
                    right = part_length;

                if (left <= right)
                    curr->push_back(part.substr(left, right-left));
                
                left = right + 1;
            }
        }
    }

    for (auto part: *curr)
        args.push_back(strdup(part.c_str()));
}
