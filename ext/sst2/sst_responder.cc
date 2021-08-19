#include "sst_responder.hh"

#include <cassert>

SSTResponder::SSTResponder(SST::SubComponent* owner_)
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
    assert(false && "SSTResponder::handleTimingReq is not implemented");
    return true;
}
