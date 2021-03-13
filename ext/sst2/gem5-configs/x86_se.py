# Copyright (c) 2021 The Regents of the University of California
# All Rights Reserved.
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
A simple system configuration with a 2-level cache.
"""

import m5
from m5.objects import *

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

def initialize_cpu(system, cpu_type_str):
    name_to_cpu_memmode_map = {
        "AtomicSimpleCPU": (AtomicSimpleCPU, "atomic"),
        "TimingSimpleCPU": (TimingSimpleCPU, "timing"),
        "DerivO3CPU": (DerivO3CPU, "timing")
    }
    CPUClass, mem_mode = name_to_cpu_memmode_map[cpu_type_str]
    system.mem_mode = mem_mode
    system.cpu = CPUClass()

if __name__ == "__m5_main__":
    args = get_args()

    system = System()

    system.clk_domain = SrcClockDomain()
    system.clk_domain.clock = '1GHz'
    system.clk_domain.voltage_domain = VoltageDomain()

    initialize_cpu(system, args.cpu_type)

    system.mem_ranges = [AddrRange('1GB')]
    system.membus = SystemXBar()

    # cpu <-> mem
    system.cpu.icache_port = system.membus.cpu_side_ports
    system.cpu.dcache_port = system.membus.cpu_side_ports
    system.cpu.createInterruptController()
    if m5.defines.buildEnv['TARGET_ISA'] == "x86":
        system.cpu.interrupts[0].pio = system.membus.mem_side_ports
        system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
        system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports
    system.mem_ctrl = MemCtrl()
    system.mem_ctrl.dram = DDR3_1600_8x8()
    system.mem_ctrl.dram.range = system.mem_ranges[0]
    system.mem_ctrl.port = system.membus.mem_side_ports
    system.system_port = system.membus.cpu_side_ports

    # Tell gem5 what the workload is
    if not args.binary:
        isa = str(m5.defines.buildEnv['TARGET_ISA']).lower()
        thispath = os.path.dirname(os.path.realpath(__file__))
        binary = os.path.join(thispath, '../../../',
                    'tests/test-progs/hello/bin/', isa, 'linux/hello')
    else:
        binary = args.binary
    system.workload = SEWorkload.init_compatible(binary)

    # Create SE process
    process = Process()
    process.cmd = [binary]
    system.cpu.workload = process
    system.cpu.createThreads()

    root = Root(full_system = False, system = system)
    m5.instantiate()

    if not args.not_simulate:
        print("Beginning simulation!")
        exit_event = m5.simulate()
        print('Exiting @ tick %i because %s' % (m5.curTick(),
              exit_event.getCause()))
