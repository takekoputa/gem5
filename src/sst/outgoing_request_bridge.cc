#include "sst/outgoing_request_bridge.hh"

#include <cassert>

namespace gem5
{

OutgoingRequestBridge::OutgoingRequestBridge(
    const OutgoingRequestBridgeParams &params) :
    SimObject(params),
    outgoingPort(std::string(name()), this),
    sstResponder(nullptr)
{
}

OutgoingRequestBridge::~OutgoingRequestBridge()
{
}

OutgoingRequestBridge::
OutgoingRequestPort::OutgoingRequestPort(const std::string &name_,
                                         OutgoingRequestBridge* owner_) :
    ResponsePort(name_, owner_)
{
    this->owner = owner_;
}

OutgoingRequestBridge::
OutgoingRequestPort::~OutgoingRequestPort()
{
}

void
OutgoingRequestBridge::init()
{
    if (this->outgoingPort.isConnected())
        this->outgoingPort.sendRangeChange();
}

Port &
OutgoingRequestBridge::getPort(const std::string &if_name, PortID idx)
{
    return this->outgoingPort;
}

AddrRangeList
OutgoingRequestBridge::getAddrRanges() const
{
    warn("OutgoingRequestBridge::getAddrRange is called");
    return outgoingPort.getAddrRanges();
}

Tick
OutgoingRequestBridge::
OutgoingRequestPort::recvAtomic(PacketPtr pkt)
{
    assert(false && "OutgoingRequestPort::recvAtomic not implemented");
    return Tick();
}

void
OutgoingRequestBridge::
OutgoingRequestPort::recvFunctional(PacketPtr pkt)
{
    assert(false && "OutgoingRequestPort::recvFunctional not implemented");
}

bool
OutgoingRequestBridge::
OutgoingRequestPort::recvTimingReq(PacketPtr pkt)
{
    owner->sstResponder->handleTimingReq(pkt);
    return true;
}

void
OutgoingRequestBridge::
OutgoingRequestPort::recvRespRetry()
{
    assert(false && "OutgoingRequestPort::recvRespRetry not implemented");
}

AddrRangeList
OutgoingRequestBridge::
OutgoingRequestPort::getAddrRanges() const
{
    return AddrRangeList({AddrRange(0x80000000, MaxAddr)});
}
}; // namespace gem5
