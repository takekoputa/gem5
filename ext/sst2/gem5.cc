#include <sst/core/sst_config.h>
#include <sst/core/componentInfo.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/memTypes.h>
#include <sst/elements/memHierarchy/util.h>

#include <Python.h>  // Before serialization to prevent spurious warnings

#include "gem5.hh"

// System headers
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
#include <sstream>

// gem5 Headers
#include <sim/core.hh>
#include <sim/init.hh>
#include <sim/init_signals.hh>
#include <sim/root.hh>
#include <sim/system.hh>
#include <sim/sim_events.hh>
#include <sim/sim_object.hh>
#include <base/logging.hh>
#include <base/debug.hh>

#include <base/pollevent.hh>
#include <base/types.hh>
#include <sim/async.hh>
#include <sim/eventq.hh>
#include <sim/sim_exit.hh>
#include <sim/stat_control.hh>

#include <sst/outgoing_request_bridge.hh>

#include <cassert>

#include <openssl/md5.h>

#ifdef fatal  // gem5 sets this
#undef fatal
#endif

// More SST Headers
#include <core/timeConverter.h>

gem5Component::gem5Component(SST::ComponentId_t id, SST::Params& params):
    SST::Component(id)
{
    output.init("gem5Component-" + getName() + "->", 1, 0, SST::Output::STDOUT);

    // Register a handler to be called on a set frequency.
    this->time_converter = registerClock(
        "1MHz",
        new SST::Clock::Handler<gem5Component>(this, &gem5Component::clockTick)
    );

    // how many gem5 cycles will be simulated within an SST clock tick
    gem5_sim_cycles = this->time_converter->getFactor();

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

    // Setting gem5 debug flags
    std::string gem5_debug_flags = params.find<std::string>("debug_flags", "");
    std::string s = gem5_debug_flags + ",";
    int prev_pos = 0;
    int pos = 0;
    output.output(CALL_INFO, "  DebugFlags = %s %d\n", s.c_str(), s.size());
    while (prev_pos < (int)s.size())
    {
        pos = s.find(",", prev_pos);
        output.output(CALL_INFO, " %d %d\n", prev_pos, pos);
        if (pos == s.npos)
            break;
        std::string debug_flag = s.substr(prev_pos, pos-prev_pos);
        gem5::setDebugFlag(debug_flag.c_str());
        output.output(CALL_INFO, "  DebugFlag += %s\n", debug_flag.c_str());
        prev_pos = pos + 1;
    }

    initPython(args.size(), &args[0]);

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    const std::vector<std::string> simobject_setup_commands = {
        "import atexit",
        "import _m5",
        "root = m5.objects.Root.getInstance()",
        "for obj in root.descendants(): obj.startup()",
        "atexit.register(m5.stats.dump)",
        "atexit.register(_m5.core.doExitCleanup)",
        "m5.stats.reset()"
    };
    this->execPythonCommands(simobject_setup_commands);

    this->system_port = loadUserSubComponent<SSTResponderSubComponent>("system_port", 0);
    this->cache_port = loadUserSubComponent<SSTResponderSubComponent>("cache_port", 0);

    system_port->setTimeConverter(this->time_converter);
    system_port->setOutputStream(&(this->output));
    cache_port->setTimeConverter(this->time_converter);
    cache_port->setOutputStream(&(this->output));

    gem5_connectors.push_back(system_port);
    gem5_connectors.push_back(cache_port);

    gem5::Root* gem5_root = gem5::Root::root();
    gem5::OutgoingRequestBridge* gem5_system_port = dynamic_cast<gem5::OutgoingRequestBridge*>(gem5_root->find("system.system_outgoing_bridge"));
    this->system_port->setResponseReceiver(gem5_system_port);
    gem5::OutgoingRequestBridge* gem5_memory_port = dynamic_cast<gem5::OutgoingRequestBridge*>(gem5_root->find("system.memory_outgoing_bridge"));
    assert(gem5_memory_port != NULL);
    this->cache_port->setResponseReceiver(gem5_memory_port);
}

gem5Component::~gem5Component()
{
}

void
gem5Component::init(unsigned phase)
{
    output.output(CALL_INFO," init phase: %u\n", phase);
    this->system_port->init(phase);
    this->cache_port->init(phase);
}

void
gem5Component::setup()
{
    output.verbose(CALL_INFO, 1, 0, "Component is being setup.\n");

    system_port->setup();
    cache_port->setup();
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

    //gem5::GlobalSimLoopExitEvent *event = gem5::simulate(gem5_sim_cycles);
    gem5::GlobalSimLoopExitEvent *event = this->simulate_gem5(gem5_sim_cycles);
    clocks_processed++;
    if (event != gem5::simulate_limit_event) { // gem5 exits due to reasons other than reaching simulation limit
        output.output("exiting: curTick()=%lu cause=`%s` code=%d\n",
            gem5::curTick(), event->getCause().c_str(), event->getCode()
        );
        primaryComponentOKToEndSim();
        return true;
    }

    // returning False means the simulation should go on
    return false;

}

#define PyCC(x) (const_cast<char *>(x))

gem5::GlobalSimLoopExitEvent*
gem5Component::simulate_gem5(gem5::Tick n_cycles)
{
    if (!(this->thread_initialized))
    {
        this->thread_initialized = true;
        gem5::simulate_limit_event = new gem5::GlobalSimLoopExitEvent(gem5::mainEventQueue[0]->getCurTick(), "simulate() limit reached", 0);
    }

    inform_once("Entering event queue @ %d.  Starting simulation...\n", gem5::curTick());

    gem5::Tick next_end_cycle = gem5::curTick() + n_cycles;
    gem5::simulate_limit_event->reschedule(next_end_cycle);
    gem5::Event *local_event = this->doSimLoop(gem5::mainEventQueue[0]);
    assert(local_event != NULL);
    gem5::BaseGlobalEvent *global_event = local_event->globalEvent();
    assert(global_event != NULL);
    gem5::GlobalSimLoopExitEvent *global_exit_event =
        dynamic_cast<gem5::GlobalSimLoopExitEvent *>(global_event);
    assert(global_exit_event != NULL);
    return global_exit_event;
}

gem5::Event*
gem5Component::doSimLoop(gem5::EventQueue* eventq)
{
    gem5::curEventQueue(eventq);
    eventq->handleAsyncInsertions();

    while (true)
    {
        // there should always be at least one event (the SimLoopExitEvent
        // we just scheduled) in the queue
        assert(!eventq->empty());
        assert(gem5::curTick() <= eventq->nextTick() &&
               "event scheduled in the past");

        if (gem5::async_event)
        {
            // Take the event queue lock in case any of the service
            // routines want to schedule new events.
            if (gem5::async_statdump || gem5::async_statreset)
            {
                gem5::statistics::schedStatEvent(gem5::async_statdump, gem5::async_statreset);
                gem5::async_statdump = false;
                gem5::async_statreset = false;
            }

            if (gem5::async_io)
            {
                gem5::async_io = false;
                gem5::pollQueue.service();
            }

            if (gem5::async_exit)
            {
                gem5::async_exit = false;
                gem5::exitSimLoop("user interrupt received");
            }

            if (gem5::async_exception)
            {
                gem5::async_exception = false;
                return NULL;
            }
        }

        gem5::Event *exit_event = eventq->serviceOne();
        if (exit_event != NULL) {
            return exit_event;
        }
    }
}

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
    gem5::initSignals();

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
        gem5::registerNativeModules();
        // initialize embedded Python interpreter
        Py_Initialize();
    }
    else {
        // https://stackoverflow.com/a/28349174
        PyImport_AddModule("_m5");
        PyObject* module = gem5::EmbeddedPyBind::initAll();
        PyObject* sys_modules = PyImport_GetModuleDict();
        PyDict_SetItemString(sys_modules, "_m5", module);
        Py_DECREF(module);
    }


    // Initialize the embedded m5 python library
    ret = gem5::EmbeddedPython::initAll();

    if (ret == 0)
    {
        // start m5
        this->startM5(argc, _argv);
    }
    else
    {
        output.output(CALL_INFO, "Not calling m5Main due to ret=%d\n", ret);
    }

    const std::vector<std::string> m5_instantiate_commands = {
        "m5.instantiate()"
    };
    this->execPythonCommands(m5_instantiate_commands);
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
