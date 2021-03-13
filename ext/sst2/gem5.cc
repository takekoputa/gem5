// Copyright (c) 2020, 2021 The Regents of the University of California
// All Rights Reserved.
//
// Copyright (c) 2015-2016 ARM Limited
// All rights reserved.
// 
// The license below extends only to copyright in the software and shall
// not be construed as granting a license to any other intellectual
// property including but not limited to intellectual property relating
// to a hardware implementation of the functionality of the software
// licensed hereunder.  You may use the software subject to the license
// terms below provided that you ensure that this notice is replicated
// unmodified and in its entirety in all distributions of the software,
// modified or unmodified, in source code or in binary form.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met: redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer;
// redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution;
// neither the name of the copyright holders nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Copyright 2009-2014 Sandia Coporation.  Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// For license information, see the LICENSE file in the current directory.

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

#ifdef fatal  // gem5 sets this
#undef fatal
#endif

// More SST Headers
#include <core/timeConverter.h>

SST::gem5::gem5Component::gem5Component(SST::ComponentId_t id,
                                        SST::Params &params) :
    SST::Component(id)
{
    dbg.init("@t:gem5:@p():@l " + getName() + ": ", 0, 0,
            (Output::output_location_t)params.find<int>("comp_debug", 0));
    info.init("gem5:" + getName() + ": ", 0, 0, Output::STDOUT);

    SST::TimeConverter *clock = registerClock(
            params.find<std::string>("frequency", "1GHz"),
            new SST::Clock::Handler<SST::gem5::gem5Component>(
                this, &SST::gem5::gem5Component::clockTick));

    // how many gem5 cycles will be simulated within an SST clock tick
    gem5_sim_cycles = clock->getFactor();

    std::string cmd = params.find<std::string>("cmd", "");
    if (cmd.empty()) {
        dbg.fatal(CALL_INFO, -1, "Component %s must have a 'cmd' parameter.\n",
               getName().c_str());
    }

    // Telling SST the command line call to gem5
    std::vector<char*> args;
    args.push_back(const_cast<char*>("sst.x")); // TODO: Compute this somehow?
    splitCommandArgs(cmd, args);
    args.push_back(const_cast<char*>("--initialize-only"));
    dbg.output(CALL_INFO, "Command string:  [sst.x %s --initialize-only]\n",
               cmd.c_str());
    for (size_t i = 0; i < args.size(); ++i) {
        dbg.output(CALL_INFO, "  Arg [%02zu] = %s\n", i, args[i]);
    }


    // Setting gem5 debug flags
    /*
    std::vector<char*> flags;
    std::string gem5DbgFlags = params.find<std::string>("gem5DebugFlags", "");
    splitCommandArgs(gem5DbgFlags, flags);
    for (auto flag : flags) {
        dbg.output(CALL_INFO, "  Setting Debug Flag [%s]\n", flag);
        setDebugFlag(flag);
    }
    */

    //ExternalMaster::registerHandler("sst", this); // these are idempotent
    //ExternalSlave ::registerHandler("sst", this);

    initPython(args.size(), &args[0]);

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    clocks_processed = 0;
}

SST::gem5::gem5Component::~gem5Component(void)
{
    Py_Finalize();
}

void
SST::gem5::gem5Component::init(unsigned phase)
{
    /*
    for (auto m : masters) {
        m->init(phase);
    }
    for (auto s : slaves) {
        s->init(phase);
    }
    */
}

void
SST::gem5::gem5Component::setup(void)
{
    // Switch connectors from initData to regular Sends
    /*
    for (auto m : masters) {
        m->setup();
    }
    for (auto s : slaves) {
        s->setup();
    }
    */
}

void
SST::gem5::gem5Component::finish(void)
{
    /*
    for (auto m : masters) {
        m->finish();
    }
    */
    info.output("Complete. Clocks Processed: %" PRIu64"\n", clocks_processed);
}

bool
SST::gem5::gem5Component::clockTick(Cycle_t cycle)
{
    // what to do in a SST's Tick

    dbg.output(CALL_INFO, "Cycle %lu\n", cycle);

    /*
    for (auto m : masters) {
        m->clock();
    }
    */

    GlobalSimLoopExitEvent *event = simulate(gem5_sim_cycles);
    ++clocks_processed;
    if (event != simulate_limit_event) {
        info.output("exiting: curTick()=%lu cause=`%s` code=%d\n",
                curTick(), event->getCause().c_str(), event->getCode());
        primaryComponentOKToEndSim();
        return true;
    }

    // returning `false` means the simulation will go on
    return false;
}


void
SST::gem5::gem5Component::splitCommandArgs(std::string &cmd, std::vector<char*> &args)
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


/**
 * Initialize the Python environment for gem5.
 * 
 * Note that, SST and gem5 will share the same Python environment. When this
 * function is called, the Python environment has already been initilized by
 * SST.  
 */
void
SST::gem5::gem5Component::initPython(int argc, char *_argv[])
{
    // should be similar to gem5's src/sim/main.cc

    PyObject *mainModule,*mainDict;

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


    if (ret == 0) {
        // start m5
        ret = m5Main(argc, _argv);
    }
    else {
        info.output(CALL_INFO, "Not calling m5Main due to ret=%d\n", ret);
    }

}

/*
ExternalMaster::Port*
gem5Component::getExternalPort(const std::string &name,
    ExternalMaster &owner, const std::string &port_data)
{
    std::string s(name); // bridges non-& result and &-arg
    auto master = new ExtMaster(this, info, owner, s);
    masters.push_back(master);
    return master;
}

ExternalSlave::Port*
gem5Component::getExternalPort(const std::string &name,
    ExternalSlave &owner, const std::string &port_data)
{
    std::string s(name); // bridges non-& result and &-arg
    auto slave = new ExtSlave(this, info, owner, s);
    slaves.push_back(slave);
    return slave;
}
*/