#ifndef __SST_OUTGOING_REQUEST_BRIDGE_HH__
#define __SST_OUTGOING_REQUEST_BRIDGE_HH__

#include "mem/external_slave.hh"
#include "mem/port.hh"
#include "params/OutgoingRequestBridge.hh"
#include "sim/sim_object.hh"
#include "sst/sst_responder_interface.hh"

namespace gem5
{

class OutgoingRequestBridge: public SimObject
{
  public:
    class OutgoingRequestPort: public ResponsePort
    {
      private:
        OutgoingRequestBridge* owner;
      public:
        OutgoingRequestPort(const std::string &name_,
                            OutgoingRequestBridge* owner_);
        ~OutgoingRequestPort();
        Tick recvAtomic(PacketPtr pkt);
        void recvFunctional(PacketPtr pkt);
        bool recvTimingReq(PacketPtr pkt);
        void recvRespRetry();
        AddrRangeList getAddrRanges() const;
    };

  public:
    OutgoingRequestPort outgoingPort;

    SSTResponderInterface* sstResponder;

    std::vector<gem5::PacketPtr> initPackets;

  public:
    OutgoingRequestBridge(const OutgoingRequestBridgeParams &params);
    ~OutgoingRequestBridge();

    void init();

    AddrRangeList getAddrRanges() const;

    Port & getPort(const std::string &if_name, PortID idx);

    std::vector<gem5::PacketPtr> getInitPackets();

    void setResponder(SSTResponderInterface* responder);

    bool sendTimingResp(gem5::PacketPtr pkt);
    void sendTimingSnoopReq(gem5::PacketPtr pkt);

    void handleRecvFunctional(gem5::PacketPtr pkt);
};

}; // namespace gem5

#endif //__SST_OUTGOING_REQUEST_BRIDGE_HH__
