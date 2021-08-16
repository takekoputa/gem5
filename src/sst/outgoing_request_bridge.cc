#include "sst/outgoing_request_bridge.hh"

#include <cassert>

namespace gem5
{

OutgoingRequestBridge::OutgoingRequestBridge(
    const OutgoingRequestBridgeParams &params) :
    SimObject(params),
    outgoingPort(std::string(name()), this)
{
}

OutgoingRequestBridge::~OutgoingRequestBridge()
{
}

void
OutgoingRequestBridge::callback_when_received()
{
}

OutgoingRequestBridge::
OutgoingRequestPort::OutgoingRequestPort(const std::string &name_,
                                         OutgoingRequestBridge* owner) :
    ResponsePort("sst", owner)
{
}

OutgoingRequestBridge::
OutgoingRequestPort::~OutgoingRequestPort()
{
}

gem5::Port &
OutgoingRequestBridge::getPort(const std::string &if_name, PortID idx)
{
    return this->outgoingPort;
}

Tick
OutgoingRequestBridge::
OutgoingRequestPort::recvAtomic(PacketPtr pkt)
{
    return Tick();
}

void
OutgoingRequestBridge::
OutgoingRequestPort::recvFunctional(PacketPtr pkt)
{
    assert("OutgoingRequestPort::recvFunctional not implemented");
}

bool
OutgoingRequestBridge::
OutgoingRequestPort::recvTimingReq(PacketPtr pkt)
{
    assert("OutgoingRequestPort::recvTimingReq not implemented");
    return true;
}

void
OutgoingRequestBridge::
OutgoingRequestPort::recvRespRetry()
{
    assert("OutgoingRequestPort::recvRespRetry not implemented");
}

AddrRangeList
OutgoingRequestBridge::
OutgoingRequestPort::getAddrRanges() const
{
    assert("OutgoingRequestPort::getAddrRanges not implemented");
    return AddrRangeList();
}
}; // namespace gem5
