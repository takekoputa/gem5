from m5.simulate import (
    scheduleTickExitAbsolute,
    scheduleTickExitFromCurrent,
    simulate,
)

from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.cachehierarchies.classic.no_cache import NoCache
from gem5.components.memory.single_channel import SingleChannelDDR3_1600
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.isas import ISA
from gem5.resources.resource import (
    Resource,
    obtain_resource,
)
from gem5.simulate.exit_handler import (
    CheckpointExitHandler,
    ExitHandler,
)
from gem5.simulate.simulator import Simulator
from gem5.utils.override import overrides

# Create the system
cache_hierarchy = NoCache()
memory = SingleChannelDDR3_1600(size="512MB")
processor = SimpleProcessor(cpu_type=CPUTypes.ATOMIC, isa=ISA.X86, num_cores=1)

# Create the board
board = SimpleBoard(
    clk_freq="1GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

# Set the workload
board.set_se_binary_workload(obtain_resource("x86-matrix-multiply"))


class UniqueCheckpointExitHandler(CheckpointExitHandler):
    # Override the process method to schedule another tick exit after a
    # checkpoint is taken.
    @overrides(CheckpointExitHandler)
    def _process(self, simulator: "Simulator") -> None:
        super()._process(simulator)
        scheduleTickExitFromCurrent(10000000000, "schedule next checkpoint")


scheduleTickExitAbsolute(10000000000, "schedule first checkpoint")

simulator = Simulator(board=board)

# The scheduler for the to-tick exit events returns type ID of one.
# Here we override it for the behavior we desire.
simulator.update_exit_handler_id_map({1: UniqueCheckpointExitHandler})

simulator.run()
