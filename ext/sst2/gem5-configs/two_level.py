# -*- coding: utf-8 -*-
# Copyright (c) 2015 Jason Power
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""
Tweaked learning_gem5 part1/two_level.py
"""

import m5
from m5.objects import *

from caches import *

import argparse

def get_args():
    parser = argparse.ArgumentParser(
        description="Simple gem5 system configuration.")
    parser.add_argument('--binary', type=str, default = '',
                        help='Path to the workload.'
                             'Hello world program by default.')
    parser.add_argument('--not-simulate', default=False, action='store_true',
                        help='Only instantiate the system, tell gem5 not to'
                             'run the simulation.')
    parser.add_argument('--cpu-type', type=str, default="AtomicSimpleCPU",
                        help='CPU type, default: AtomicSimpleCPU')
    parser.add_argument('--allow-listeners', default=False,
                        action='store_true',
                        help='Allow listeners to connect to gem5.')
    return parser.parse_args()

if __name__ == "__m5_main__":
    args = get_args()

    # Tell gem5 what the workload is
    if not args.binary:
        isa = str(m5.defines.buildEnv['TARGET_ISA']).lower()
        thispath = os.path.dirname(os.path.realpath(__file__))
        binary = os.path.join(thispath, '../../../',
                    'tests/test-progs/hello/bin/', isa, 'linux/hello')
    else:
        binary = args.binary

    system = System()

    system.clk_domain = SrcClockDomain()
    system.clk_domain.clock = '1GHz'
    system.clk_domain.voltage_domain = VoltageDomain()

    system.mem_mode = 'timing'               # Use timing accesses
    system.mem_ranges = [AddrRange('512MB')] # Create an address range

    # Create a simple CPU
    system.cpu = TimingSimpleCPU()

    system.cpu.icache = L1ICache()
    system.cpu.dcache = L1DCache()

    # Connect the instruction and data caches to the CPU
    system.cpu.icache.connectCPU(system.cpu)
    system.cpu.dcache.connectCPU(system.cpu)

    # Create a memory bus, a coherent crossbar, in this case
    system.l2bus = L2XBar()

    # Hook the CPU ports up to the l2bus
    system.cpu.icache.connectBus(system.l2bus)
    system.cpu.dcache.connectBus(system.l2bus)

    # Create an L2 cache and connect it to the l2bus
    system.l2cache = L2Cache()
    system.l2cache.connectCPUSideBus(system.l2bus)

    # Create a memory bus
    system.membus = SystemXBar()

    # Connect the L2 cache to the membus
    system.l2cache.connectMemSideBus(system.membus)

    # create the interrupt controller for the CPU
    system.cpu.createInterruptController()

    # For x86 only, make sure the interrupts are connected to the memory
    # Note: these are directly connected to the memory bus and are not cached
    if m5.defines.buildEnv['TARGET_ISA'] == "x86":
        system.cpu.interrupts[0].pio = system.membus.mem_side_ports
        system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
        system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

    # Connect the system up to the membus
    system.system_port = system.membus.cpu_side_ports

    # Create a DDR3 memory controller
    system.mem_ctrl = MemCtrl()
    system.mem_ctrl.dram = DDR3_1600_8x8()
    system.mem_ctrl.dram.range = system.mem_ranges[0]
    system.mem_ctrl.port = system.membus.mem_side_ports

    system.workload = SEWorkload.init_compatible(binary)

    # Create a process for a simple "Hello World" application
    process = Process()
    # Set the command
    # cmd is a list which begins with the executable (like argv)
    process.cmd = [binary]
    # Set the cpu to use the process as its workload and create thread contexts
    system.cpu.workload = process
    system.cpu.createThreads()

    # set up the root SimObject and start the simulation
    root = Root(full_system = False, system = system)
    # instantiate all of the objects we've created above
    m5.instantiate()

    if not args.not_simulate:
        print("Beginning simulation!")
        exit_event = m5.simulate()
        print('Exiting @ tick %i because %s' % (m5.curTick(), exit_event.getCause()))
