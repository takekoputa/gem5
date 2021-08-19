#ifndef __TRANSLATOR_H__
#define __TRANSLATOR_H__

#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/memTypes.h>
#include <sst/elements/memHierarchy/util.h>

namespace Translator
{


SST::MemHierarchy::MemEvent*
gem5RequestToSSTMemEvent(std::string comp, gem5::PacketPtr ptk)
{
    SST::MemHierarchy::Command cmd = SST::MemHierarchy::Command::GetX;

    SST::MemHierarchy::MemEvent* mem_event = new SST::MemHierarchy::MemEvent(
        comp, ptk->getAddr(), ptk->getAddr(), cmd);

    return mem_event;
}

}; // namespace Translator

#endif // __TRANSLATOR_H__