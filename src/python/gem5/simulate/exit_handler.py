from abc import abstractmethod
from pathlib import Path
from typing import (
    Any,
    Dict,
)

import m5
from m5 import options
from m5.util import warn

from gem5.simulate.exit_event import ExitEvent
from gem5.utils.override import overrides


class ExitHandler:
    def __init__(self, payload: Dict[str, str]) -> None:
        self._payload = payload

    def handle(self, simulator: "Simulator") -> bool:
        self._process(simulator)
        return self._exit_simulation()

    @abstractmethod
    def _process(self, simulator: "Simulator") -> None:
        raise NotImplementedError(
            "Method '_process' must be implemented by a subclass"
        )

    @abstractmethod
    def _exit_simulation(self) -> bool:
        raise NotImplementedError(
            "Method '_exit_simulation' must be implemented by a subclass"
        )


class ClassicGeneratorExitHandler(ExitHandler):

    def __init__(self, payload: Dict[str, str]) -> None:
        super().__init__(payload)
        self._exit_on_completion = None

    @overrides(ExitHandler)
    def _process(self, simulator: "Simulator") -> None:
        #  # Translate the exit event cause to the exit event enum.
        exit_enum = ExitEvent.translate_exit_status(
            simulator.get_last_exit_event_cause()
        )

        # Check to see the run is corresponding to the expected execution
        # order (assuming this check is demanded by the user).
        if simulator._expected_execution_order:
            expected_enum = simulator._expected_execution_order[
                simulator._exit_event_count
            ]
            if exit_enum.value != expected_enum.value:
                raise Exception(
                    f"Expected a '{expected_enum.value}' exit event but a "
                    f"'{exit_enum.value}' exit event was encountered."
                )

        # Record the current tick and exit event enum.
        simulator._tick_stopwatch.append(
            (exit_enum, simulator.get_current_tick())
        )

        try:
            # If the user has specified their own generator for this exit
            # event, use it.
            self._exit_on_completion = next(
                simulator._on_exit_event[exit_enum]
            )
        except StopIteration:
            # If the user's generator has ended, throw a warning and use
            # the default generator for this exit event.
            warn(
                "User-specified generator/function list for the exit "
                f"event'{exit_enum.value}' has ended. Using the default "
                "generator."
            )
            self._exit_on_completion = next(
                simulator._default_on_exit_dict[exit_enum]
            )

        except KeyError:
            # If the user has not specified their own generator for this
            # exit event, use the default.
            self._exit_on_completion = next(
                simulator._default_on_exit_dict[exit_enum]
            )

    @overrides(ExitHandler)
    def _exit_simulation(self) -> bool:
        assert (
            self._exit_on_completion is not None
        ), "Exit on completion boolean var is not set."
        return self._exit_on_completion


class IgnoreAndContinueExitHandler(ExitHandler):

    @overrides(ExitHandler)
    def _process(self, simulator: "Simulator") -> None:
        from m5 import info

        info(
            f"To Tick Exit triggered at tick: {m5.curTick()}"
            f"To Tick Exit was scheduled at tick: {self._payload().get('scheduled_at_tick', 'NOT_SET')}"
            f"To Tick Exit string: {self._payload.get('exit_string', 'NOT_SET')}"
        )

    @overrides(ExitHandler)
    def _exit_simulation(self) -> bool:
        return True


class KernelBootedExitHandler(ExitHandler):
    @overrides(ExitHandler)
    def _process(self, simulator: "Simulator") -> None:
        print("First exit: kernel booted")

    @overrides(ExitHandler)
    def _exit_simulation(self) -> bool:
        return False


class AfterBootExitHandler(ExitHandler):
    @overrides(ExitHandler)
    def _process(self, simulator: "Simulator") -> None:
        print("Second exit: Started `after_boot.sh` script")
        print("Switching to Timing CPU")

    @overrides(ExitHandler)
    def _exit_simulation(self) -> bool:
        return False


class AfterBootScriptExitHandler(ExitHandler):
    @overrides(ExitHandler)
    def _process(self, simulator: "Simulator") -> None:
        print("Third exit: Finished `after_boot.sh` script")

    @overrides(ExitHandler)
    def _exit_simulation(self) -> bool:
        return True


class ToTickExitHandler(ExitHandler):
    @overrides(ExitHandler)
    def _process(self, simulator: "Simulator") -> None:
        checkpoint_dir = Path(simulator._checkpoint_path)
        if not checkpoint_dir:
            checkpoint_dir = Path(options.outdir)
        m5.checkpoint((checkpoint_dir / f"cpt.{str(m5.curTick())}").as_posix())

    @overrides(ExitHandler)
    def _exit_simulation(self) -> bool:
        return True


class CheckpointExitHandler(ExitHandler):

    @overrides(ExitHandler)
    def _process(self, simulator: "Simulator") -> None:
        checkpoint_dir = simulator._checkpoint_path
        if not checkpoint_dir:
            checkpoint_dir = options.outdir
        m5.checkpoint(
            (Path(checkpoint_dir) / f"cpt.{str(m5.curTick())}").as_posix()
        )

    @overrides(ExitHandler)
    def _exit_simulation(self) -> bool:
        return False
