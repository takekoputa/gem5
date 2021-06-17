import m5
from m5.objects import *
from os import path

ExternalResponder = ExternalSlave

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

def createHiFivePlatform(system):
    system.internal_membus = SystemXBar()
    system.internal_membus.badaddr_responder = BadAddr()
    system.internal_membus.default = \
        system.internal_membus.badaddr_responder.pio

    for cpu in system.cpu:
        cpu.createThreads()
        system.cpu_xbar = SystemXBar()
        system.icache_outgoing_bridge = OutgoingRequestBridge(connector_name = "icache_port") # connector_name = x -> an SST object named "icache_port" will look for this bridge
        cpu.icache_port = system.icache_outgoing_bridge.port
        cpu.dcache_port = system.cpu_xbar.cpu_side_ports
        cpu.mmu.connectWalkerPorts(
            system.internal_membus.cpu_side_ports, system.internal_membus.cpu_side_ports)
        system.dcache_outgoing_bridge = OutgoingRequestBridge(connector_name = "dcache_port")
        system.cpu_xbar.mem_side_ports = system.dcache_outgoing_bridge.port
        system.cpu_xbar.mem_side_ports = system.internal_membus.cpu_side_ports

    system.platform = HiFive()

    system.platform.rtc = RiscvRTC(frequency=Frequency("100MHz"))
    system.platform.clint.int_pin = system.platform.rtc.int_pin
    system.platform.attachPlic()

    system.pma_checker = PMAChecker(
        uncacheable=[
            *system.platform._on_chip_ranges(),
            *system.platform._off_chip_ranges(),
        ]
    )

    system.platform.attachOnChipIO(system.internal_membus)
    system.platform.attachOffChipIO(system.internal_membus)

    print("Done")



system = System()

system.clk_domain = SrcClockDomain(
    clock="3GHz", voltage_domain=VoltageDomain()
)

system.mem_ranges = [AddrRange(start=0x80000000, size="512MB")]

system.cpu = [TimingSimpleCPU(cpu_id=i) for i in range(1)]
system.mem_mode = 'timing'

#system.intrctrl = IntrControl()

createHiFivePlatform(system)

system.system_outgoing_bridge = OutgoingRequestBridge(connector_name = "system_port")
system.system_port = system.system_outgoing_bridge.port

generateDtb(system)
system.workload.dtb_filename = path.join(m5.options.outdir, 'device.dtb')
system.workload.dtb_addr = 0x87e00000 # how to determine this?
kernel_cmd = [
    "console=ttyS0",
    "root=/dev/vda",
    "ro"
]
system.workload.command_line = " ".join(kernel_cmd)

for cpu in system.cpu:
    cpu.createInterruptController()

root = Root(full_system = True, system = system)

