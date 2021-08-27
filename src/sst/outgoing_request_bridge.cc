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
    return outgoingPort.getAddrRanges();
}

std::vector<gem5::PacketPtr>
OutgoingRequestBridge::getInitPackets()
{
    return this->initPackets;
}

void
OutgoingRequestBridge::setResponder(SSTResponderInterface* responder)
{
    this->sstResponder = responder;
}

bool
OutgoingRequestBridge::sendTimingResp(gem5::PacketPtr pkt)
{
    return this->outgoingPort.sendTimingResp(pkt);
}

void
OutgoingRequestBridge::sendTimingSnoopReq(gem5::PacketPtr pkt)
{
    this->outgoingPort.sendTimingSnoopReq(pkt);
}

void
OutgoingRequestBridge::handleRecvFunctional(gem5::PacketPtr pkt)
{
    gem5::PacketPtr pkt_clone = new gem5::Packet(pkt, false, true);
    this->initPackets.push_back(pkt_clone);
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
    owner->handleRecvFunctional(pkt);
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
    //assert(false && "OutgoingRequestPort::recvRespRetry not implemented");
    owner->sstResponder->handleRecvRespRetry();
}

AddrRangeList
OutgoingRequestBridge::
OutgoingRequestPort::getAddrRanges() const
{
    return AddrRangeList({AddrRange(0x80000000, MaxAddr)});
}
}; // namespace gem5
