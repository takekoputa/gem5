#ifndef __SST_OUTGOING_REQUEST_BRIDGE_HH__
#define __SST_OUTGOING_REQUEST_BRIDGE_HH__

#include "mem/external_slave.hh"
#include "mem/port.hh"
#include "params/OutgoingRequestBridge.hh"
#include "sim/sim_object.hh"

//namespace gem5
//{

class OutgoingRequestBridge: public gem5::SimObject
{
  public:
    class OutgoingRequestPort: public gem5::ResponsePort
    {
      public:
        OutgoingRequestPort(const std::string &name_,
                            OutgoingRequestBridge* owner);
        ~OutgoingRequestPort();
        gem5::Tick recvAtomic(gem5::PacketPtr pkt);
        void recvFunctional(gem5::PacketPtr pkt);
        bool recvTimingReq(gem5::PacketPtr pkt);
        void recvRespRetry();
        gem5::AddrRangeList getAddrRanges() const;
    };

  public:
    OutgoingRequestPort outgoingPort;

  public:
    OutgoingRequestBridge(const gem5::OutgoingRequestBridgeParams &params);
    ~OutgoingRequestBridge();

    gem5::Port & getPort(const std::string &if_name, gem5::PortID idx);

    void callback_when_received();
};

//}; // namespace gem5

#endif //__SST_OUTGOING_REQUEST_BRIDGE_HH__
