#include <core/sst_config.h>
#include <Python.h>  // Before serialization to prevent spurious warnings

#include "gem5.hh"

// System headers
#include <string>

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
    registerClock(
        "1MHz",
        new SST::Clock::Handler<gem5Component>(this, &gem5Component::clockTick)
    );

    // "cmd" -> gem5's Python
    std::string cmd = params.find<std::string>("cmd", "");
    if (cmd.empty()) {
        output.fatal(
            CALL_INFO, -1, "Component %s must have a 'cmd' parameter.\n",
            getName().c_str()
        );
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
    primaryComponentOKToEndSim();
    return true;
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
        ret = m5Main(argc, _argv);
    }
    else
    {
        output.output(CALL_INFO, "Not calling m5Main due to ret=%d\n", ret);
    }
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
