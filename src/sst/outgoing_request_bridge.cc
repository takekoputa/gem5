#include "sst/outgoing_request_bridge.hh"

OutgoingRequestBridge::OutgoingRequestBridge(
    const OutgoingRequestBridgeParams& params) :
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

Port &
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
}

bool
OutgoingRequestBridge::
OutgoingRequestPort::recvTimingReq(PacketPtr pkt)
{
    return true;
}

void
OutgoingRequestBridge::
OutgoingRequestPort::recvRespRetry()
{
}

AddrRangeList
OutgoingRequestBridge::
OutgoingRequestPort::getAddrRanges() const
{
    return AddrRangeList();
}
