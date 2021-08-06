#include "sst/outgoing_request_bridge.hh"

//namespace gem5
//{

OutgoingRequestBridge::OutgoingRequestBridge(
    const gem5::OutgoingRequestBridgeParams &params) :
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
OutgoingRequestBridge::getPort(const std::string &if_name, gem5::PortID idx)
{
    return this->outgoingPort;
}

gem5::Tick
OutgoingRequestBridge::
OutgoingRequestPort::recvAtomic(gem5::PacketPtr pkt)
{
    return gem5::Tick();
}

void
OutgoingRequestBridge::
OutgoingRequestPort::recvFunctional(gem5::PacketPtr pkt)
{
}

bool
OutgoingRequestBridge::
OutgoingRequestPort::recvTimingReq(gem5::PacketPtr pkt)
{
    return true;
}

void
OutgoingRequestBridge::
OutgoingRequestPort::recvRespRetry()
{
}

gem5::AddrRangeList
OutgoingRequestBridge::
OutgoingRequestPort::getAddrRanges() const
{
    return gem5::AddrRangeList();
}
//}; // namespace gem5
