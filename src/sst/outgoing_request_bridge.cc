#include "sst/outgoing_request_bridge.hh"

#include <cassert>
#include <iomanip>
#include <sstream>

#include "base/trace.hh"
#include "debug/SST.hh"

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

std::unordered_map<Addr, std::vector<uint8_t>>
OutgoingRequestBridge::getInitData()
{
    return this->initData;
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
OutgoingRequestBridge::handleRecvFunctional(PacketPtr pkt)
{
    std::stringstream s;
    s << "gem5 wants to send to " << std::hex << pkt->getAddr() << " data: [";
    auto ptr = pkt->getPtr<uint8_t>();
    for (unsigned int k = 0; k < pkt->getSize(); k++)
    {
        uint64_t data = (uint64_t)(*ptr);
        s << std::hex <<data << ",";
        ptr++;
    }
    s << "]\n";
    DPRINTF(SST, "%s", s.str().c_str());
    std::vector<uint8_t> data;
    auto ptr = pkt->getPtr<uint8_t>();
    for (unsigned int k = 0; k < pkt->getSize(); k++)
    {
        data.push_back(*ptr);
        ptr++;
    }
    this->initData[pkt->getAddr()] = data;
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
