import m5
from m5.objects import *
from os import path

def generateMemNode(state, mem_range):
    node = FdtNode("memory@%x" % int(mem_range.start))
    node.append(FdtPropertyStrings("device_type", ["memory"]))
    node.append(FdtPropertyWords("reg",
        state.addrCells(mem_range.start) +
        state.sizeCells(mem_range.size()) ))
    return node

def generateDtb(system):
    """
    Autogenerate DTB. Arguments are the folder where the DTB
    will be stored, and the name of the DTB file.
    """
    state = FdtState(addr_cells=2, size_cells=2, cpu_cells=1)
    root = FdtNode('/')
    root.append(state.addrCellsProperty())
    root.append(state.sizeCellsProperty())
    root.appendCompatible(["riscv-virtio"])

    for mem_range in system.mem_ranges:
        root.append(generateMemNode(state, mem_range))

    sections = [*system.cpu, system.platform]

    for section in sections:
        for node in section.generateDeviceTree(state):
            if node.get_name() == root.get_name():
                root.merge(node)
            else:
                root.append(node)

    fdt = Fdt()
    fdt.add_rootnode(root)
    fdt.writeDtsFile(path.join(m5.options.outdir, 'device.dts'))
    fdt.writeDtbFile(path.join(m5.options.outdir, 'device.dtb'))

system = System()

system.clk_domain = SrcClockDomain(
    clock="3GHz", voltage_domain=VoltageDomain()
)

system.mem_ranges = [AddrRange(start=0x80000000, size="512MB")]

system.membus = SystemXBar()
system.membus.badaddr_responder = BadAddr()
system.membus.default = system.membus.badaddr_responder.pio

system.system_port = system.membus.cpu_side_ports

system.cpu = [AtomicSimpleCPU(cpu_id=i) for i in range(1)]

for cpu in system.cpu:
    cpu.createThreads()
    cpu.icache_port = system.membus.cpu_side_ports
    cpu.dcache_port = system.membus.cpu_side_ports
    cpu.mmu.connectWalkerPorts(
           system.membus.cpu_side_ports, system.membus.cpu_side_ports)

# Broken
system.workload = RiscvLinux(object_file='bbl-no-dts')

system.platform = HiFive()

# Skipping adding a disk

system.platform.rtc = RiscvRTC(frequency=Frequency("100MHz"))
system.platform.clint.int_pin = system.platform.rtc.int_pin
system.platform.attachPlic()

system.pma_checker = PMAChecker(
    uncacheable=[
        *system.platform._on_chip_ranges(),
        *system.platform._off_chip_ranges(),
    ]
)

system.iobus = IOXBar()

system.bridge = Bridge(delay="50ns")
system.membus.mem_side_ports = system.bridge.cpu_side_port
system.bridge.mem_side_port = system.iobus.cpu_side_ports
system.bridge.ranges = system.platform._off_chip_ranges()
# Connecting on chip and off chip IO to the mem
# and IO bus
system.platform.attachOnChipIO(system.membus)
system.platform.attachOffChipIO(system.iobus)

system.mem_ctrl = SimpleMemory(range=system.mem_ranges[0])
system.mem_ctrl.port = system.membus.mem_side_ports

generateDtb(system)
system.workload.dtb_filename = path.join(m5.options.outdir, 'device.dtb')
# Default DTB address if bbl is bulit with --with-dts option
system.workload.dtb_addr = 0x87e00000

kernel_cmd = [
    "console=ttyS0",
    "root=/dev/vda",
    "ro"
]
system.workload.command_line = " ".join(kernel_cmd)

for cpu in system.cpu:
    cpu.createInterruptController()

root = Root(full_system = True, system = system)

m5.instantiate()

print("Running the simulation")
exit_event = m5.simulate()

print(f"{exit_event.getCause()} @ {m5.curTick()}")
