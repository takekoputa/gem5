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

bool
SSTResponder::handleTimingReq(gem5::PacketPtr pkt)
{
    auto mem_event = Translator::gem5RequestToSSTMemEvent(this->owner->getName(), pkt);
    return owner->handleTimingReq(mem_event);
}
