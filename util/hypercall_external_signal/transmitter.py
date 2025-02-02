#!/usr/bin/env python3

# Copyright (c) 2023 The Regents of the University of California
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

import json
import os
import signal
import sys
from multiprocessing import shared_memory
from time import sleep
from typing import Optional


def send_signal(pid: int, id: int, payload: str) -> None:
    shared_mem_name = "shared_gem5_signal_mem_" + str(pid)
    shared_mem_size = 4096
    try:
        shm = shared_memory.SharedMemory(
            name=shared_mem_name, create=True, size=shared_mem_size
        )
    except FileExistsError:
        shm = shared_memory.SharedMemory(name=shared_mem_name)

    shm.buf[:shared_mem_size] = b"\x00" * shared_mem_size
    final_payload = create_json(id, payload)
    shm.buf[: len(final_payload.encode())] = final_payload.encode()
    try:
        # Note: SIGHUP is used as SIGUSR1 and SIGUSR2 are already in used by
        # gem5 for other purposes. SIGRTMIN and SIGRTMAX (usually the suggested
        # alternative when SIGUSR1 and SIGUSR2 unavailable) cannot be used in
        # this case as they are not supported on MacOS.
        #
        # SIGHUP is compatible with both Linux and MacOS and was not otherwise
        # used by gem5. In general, SIGHUP is a signal that is
        # for use in interacting with system daemons to request reloading of
        # their configurations.
        os.kill(pid, signal.SIGHUP)
    except ProcessLookupError:
        print(
            "Process does not exist! Check that you are using the correct PID."
        )
        shm.close()
        shm.unlink()
        sys.exit(1)

    print(f"Sent a SIGHUP signal to PID {pid} with payload: '{final_payload}'")

    while bytes(shm.buf[:shared_mem_size]).decode().strip("\x00") != "done":
        print("Waiting for gem5 to finish using shared memory...")
        sleep(1)
    print("Done message received")
    shm.close()
    try:
        shm.unlink()
    except FileNotFoundError:
        pass


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python sender.py <PID> <Hypercall ID> <Payload>")
        sys.exit(1)


def create_json(id: int, payload: Optional[str] = "{}") -> str:
    try:
        payload_dict = json.loads(payload)
    except:
        raise Exception(
            "Failed to read string as JSON! Try checking its formatting."
        )
    final_dict = {"id": id, "payload": payload_dict}
    return json.dumps(final_dict)


print(sys.argv)

if len(sys.argv) == 4:
    send_signal(int(sys.argv[1]), int(sys.argv[2]), sys.argv[3])
else:
    send_signal(int(sys.argv[1]), int(sys.argv[2]), "{}")
