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
    auto request = Translator::gem5RequestToSSTRequest(
        pkt, this->owner->sst_request_id_to_packet_map);
    //mem_event->setDst("l1_cache");
    return owner->handleTimingReq(request);
}

void
SSTResponder::handleRecvRespRetry()
{
    this->owner->handleRecvRespRetry();
}

void
SSTResponder::handleRecvFunctional(gem5::PacketPtr pkt)
{
    this->owner->handleRecvFunctional(pkt);
}

std::string
SSTResponder::getName()
{
    return this->owner->getName() + ".port";
}
