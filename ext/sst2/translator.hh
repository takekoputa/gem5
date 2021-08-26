#ifndef __TRANSLATOR_H__
#define __TRANSLATOR_H__

#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/memTypes.h>
#include <sst/elements/memHierarchy/util.h>

struct TSSTEventIdHash
{
    size_t operator() (const SST::Event::id_type& p) const
    {
        return (uint64_t)(p.first) ^ (uint64_t)(p.second);
    }

};


typedef std::unordered_map<SST::Event::id_type, gem5::PacketPtr, TSSTEventIdHash> TPacketMap;

namespace Translator
{

inline SST::MemHierarchy::MemEvent*
gem5RequestToSSTMemEvent(std::string comp, gem5::PacketPtr pkt,
                         TPacketMap& event_id_to_packet_map)
{
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

    if (pkt->needsResponse())
        event_id_to_packet_map[ev->getID()] = pkt;

    return ev;
}

inline void
inplaceSSTMemEventToGem5PacketPtr(gem5::PacketPtr pkt,
                                  SST::MemHierarchy::MemEvent* event)
{   
    pkt->makeResponse();  // Convert to a response packet
    pkt->setData(event->getPayload().data());

    // Resolve the success of Store Conditionals
    if (pkt->isLLSC() && pkt->isWrite()) {
        //pkt->req->setExtraData(event->isAtomic());
    }

    // Clear out bus delay notifications
    pkt->headerDelay = pkt->payloadDelay = 0;
}

}; // namespace Translator

#endif // __TRANSLATOR_H__
