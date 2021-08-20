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
gem5RequestToSSTMemEvent(std::string comp, gem5::PacketPtr pkt)
{
//    SST::MemHierarchy::Command cmd = SST::MemHierarchy::Command::GetS;

//    SST::MemHierarchy::MemEvent* mem_event = new SST::MemHierarchy::MemEvent(
//        comp, ptk->getAddr(), ptk->getAddr(), cmd);

    SST::MemHierarchy::Command cmd;
    switch ((gem5::MemCmd::Command)pkt->cmd.toInt()) {
        case gem5::MemCmd::HardPFReq:
        case gem5::MemCmd::SoftPFReq:
        case gem5::MemCmd::LoadLockedReq:
        case gem5::MemCmd::ReadExReq:
        case gem5::MemCmd::ReadReq:
            cmd = SST::MemHierarchy::Command::GetS;
            break;
        case gem5::MemCmd::StoreCondReq:
        case gem5::MemCmd::WriteReq:
            cmd = SST::MemHierarchy::Command::GetX;
            break;
        default:
            //out.fatal(CALL_INFO, 1, "Don't know how to convert gem5 packet "
            //      "command %s to SST\n", pkt->cmd.toString().c_str());
            assert(false && "Unable to convert gem5 packet");
    }

    SST::MemHierarchy::MemEvent* ev = new SST::MemHierarchy::MemEvent(comp, pkt->getAddr(), pkt->getAddr(), cmd);
    ev->setPayload(pkt->getSize(), pkt->getPtr<uint8_t>());
    if ((gem5::MemCmd::Command)pkt->cmd.toInt() == gem5::MemCmd::LoadLockedReq)
    {
        ev->setLoadLink();
    }
    else if ((gem5::MemCmd::Command)pkt->cmd.toInt() == gem5::MemCmd::StoreCondReq)
    {
        ev->setStoreConditional();
    }

    if (pkt->req->isLockedRMW())
        ev->setFlag(SST::MemHierarchy::MemEvent::F_LOCKED);
    if (pkt->req->isUncacheable())
        ev->setFlag(SST::MemHierarchy::MemEvent::F_NONCACHEABLE);
//    if (pkt->req->hasContextId())
//        ev->setGroupId(pkt->req->contextId());
// Prefetches not working with SST; it maybe be dropping them, treating them
// as not deserving of responses, or something else -- not sure yet.
//  ev->setPrefetchFlag(pkt->req->isPrefetch());

//    if (pkt->needsResponse())
//        PacketMap[ev->getID()] = pkt;

    return ev;
}

}; // namespace Translator

#endif // __TRANSLATOR_H__
