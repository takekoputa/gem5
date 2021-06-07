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
    parser.add_argument('--cpu-type', type=str, default="TimingSimpleCPU",
                        help='CPU type, default: TimingSimpleCPU')
    parser.add_argument('--allow-listeners', default=False,
                        action='store_true',
                        help='Allow listeners to connect to gem5.')
    return parser.parse_args()

def initialize_cpu(system, cpu_type_str):
    name_to_cpu_memmode_map = {
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

    system.membus = SystemXBar()
    system.membus_icache = SystemXBar()
    system.membus_dcache = SystemXBar()

    addr_ranges = [AddrRange('3GB')]

    # cpu <-> mem
    system.external_icache = ExternalSlave(
        port_type = "sst_icache",
        port_data = "init_mem0", # ???
        port = system.membus_icache.mem_side_ports,
        addr_ranges = addr_ranges
    )
    system.external_dcache = ExternalSlave(
        port_type = "sst_dcache",
        port_data = "init_mem0",
        port = system.membus_dcache.mem_side_ports,
        addr_ranges = addr_ranges
    )
    
    system.cpu.icache_port = system.membus_icache.cpu_side_ports
    system.cpu.dcache_port = system.membus_dcache.cpu_side_ports
    system.cpu.createInterruptController()
    if m5.defines.buildEnv['TARGET_ISA'] == "x86":
        system.cpu.interrupts[0].pio = system.membus.mem_side_ports
        system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
        system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

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