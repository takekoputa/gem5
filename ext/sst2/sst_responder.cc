#include "sst_responder.hh"

#include <cassert>

#include "translator.hh"

SSTResponder::SSTResponder(SSTResponderSubComponent* owner_)
    : gem5::SSTResponderInterface()
{
    this->owner = owner_;
}

SSTResponder::~SSTResponder()
{
}

void
SSTResponder::setOutputStream(SST::Output* output_)
{
    this->output = output_;
}

bool
SSTResponder::handleTimingReq(gem5::PacketPtr pkt)
{
    auto mem_event = Translator::gem5RequestToSSTMemEvent(
        this->owner->getName(), pkt, this->owner->event_id_to_packet_map);
    mem_event->setDst("l1_cache");
    return owner->handleTimingReq(mem_event);
}

void
SSTResponder::handleRecvRespRetry()
{
    owner->handleRecvRespRetry();
}


std::string
SSTResponder::getName()
{
    return owner->getName() + ".port";
}
