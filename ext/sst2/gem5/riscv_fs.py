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

"""
def createHiFivePlatform(system):
    system.internal_membus = SystemXBar()

    system.on_chip_devices = {
        "clint": Clint(pio_addr=0x2000000),
        "plic" = Plic(pio_add=0xc000000),
        "uart" = Uart8250(pio_addr=0x10000000) # not on chip?
    } 

    # mem_side_ports of internal_membus
    system.rtc = RiscvRTC(frequency=Frequency("100MHz"))
    system.clint = system.on_chip_devices["clint"]
    system.internal_membus.mem_side_ports = system.clint.pio
    system.rtc.int_pin = system.clint.int_pin

    system.plic = system.on_chip_devices["plic"]
    system.internal_membus.mem_side_ports = system.plic.pio

    system.uart = system.on_chip_devices["uart"]
    system.internal_membus.mem_side_ports = system.uart.pio

    # cpu_side_ports of internal_membus
    system.system_port = OutgoingRequestBridge()

    system.cpu_xbar = []

    for n, cpu in enumerate(system.cpu):
        cpu.createThreads()
        cpu.createInterruptController()
        cpu_xbar = SystemXBar()
        cpu.icache_port = OutgoingRequestBridge()
        cpu.dcache_port = cpu_xbar.cpu_side_ports
        cpu.mmu.connectWalkerPorts(
            system.internal_membus.cpu_side_ports,
            system.internal_membus.cpu_side_ports)
        cpu_xbar.mem_side_ports = OutgoingRequestBridge()
        cpu_xbar.mem_side_ports = system.internal_membus.cpu_side_ports
        system.cpu_xbar.append(cpu_xbar)
    
    uncacheable_ranges = []
    for _, dev in system.on_chip_devices.items():
        uncacheable_ranges.append(AddrRange(dev.pio_addr, size=dev.pio_size))

    system.pma_checker = PMAChecker(uncacheable=uncacheable_ranges)
"""
def createHiFivePlatform(system):
    system.internal_membus = SystemXBar()
    system.internal_membus.badaddr_responder = BadAddr()
    system.internal_membus.default = \
        system.internal_membus.badaddr_responder.pio

    #system.cpu_xbars = []

    for n, cpu in enumerate(system.cpu):
        cpu.createThreads()
        cpu_xbar = SystemXBar()
        cpu.icache_port = OutgoingRequestBridge().port
        cpu.dcache_port = cpu_xbar.cpu_side_ports
        cpu.mmu.connectWalkerPorts(
            system.internal_membus.cpu_side_ports,
            system.internal_membus.cpu_side_ports)
        cpu_xbar.mem_side_ports = OutgoingRequestBridge().port
        cpu_xbar.mem_side_ports = system.internal_membus.cpu_side_ports
        #system.cpu_xbars.append(cpu_xbar)

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

system.intrctrl = IntrControl()

createHiFivePlatform(system)

generateDtb(system)
system.workload.dtb_filename = path.join(m5.options.outdir, 'device.dtb')
system.workload.dtb_addr = 0x80000000 # how to determine this?
kernel_cmd = [
    "console=ttyS0",
    "root=/dev/vda",
    "ro"
]
system.workload.command_lline = " ".join(kernel_cmd)

for cpu in system.cpu:
    cpu.createInterruptController()

root = Root(full_system = True, system = system)

