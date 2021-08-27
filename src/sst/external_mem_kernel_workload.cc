/*
 * Copyright 2019 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sst/external_mem_kernel_workload.hh"

#include "debug/Loader.hh"
#include "params/ExternalMemKernelWorkload.hh"
#include "sim/kernel_workload.hh"
#include "sim/system.hh"

namespace gem5
{

ExternalMemKernelWorkload::ExternalMemKernelWorkload(const Params &p) :
    KernelWorkload(p)
{
}

void
ExternalMemKernelWorkload::initState()
{
    auto &phys_mem = system->physProxy;
    /**
     * Load the kernel code into memory.
     */
    auto mapper = [this](Addr a) {
        return (a & _loadAddrMask) + _loadAddrOffset;
    };
    if (params().object_file != "")  {
        if (params().addr_check) {
            // Validate kernel mapping before loading binary
            fatal_if(!system->isMemAddr(mapper(_start)) ||
                    !system->isMemAddr(mapper(_end)),
                    "Kernel is mapped to invalid location (not memory). "
                    "start (%#x) - end (%#x) %#x:%#x\n",
                    _start, _end, mapper(_start), mapper(_end));
        }
        // Load program sections into memory
        image.write(phys_mem);
        DPRINTF(Loader, "Kernel start = %#x\n", _start);
        DPRINTF(Loader, "Kernel end   = %#x\n", _end);
        DPRINTF(Loader, "Kernel entry = %#x\n", kernelObj->entryPoint());
    }

    std::vector<Addr> extras_addrs = params().extras_addrs;
    if (extras_addrs.empty())
        extras_addrs.resize(params().extras.size(), MaxAddr);
    for (int idx = 0; idx < extras.size(); idx++) {
        const Addr load_addr = extras_addrs[idx];
        auto image = extras[idx]->buildImage();
        if (load_addr != MaxAddr)
            image = image.offset(load_addr);
        else
            image = image.move(mapper);
        image.write(phys_mem);
    }
}


} // namespace gem5
