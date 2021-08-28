#ifndef __SST_OUTGOING_REQUEST_BRIDGE_HH__
#define __SST_OUTGOING_REQUEST_BRIDGE_HH__

#include <unordered_map>
#include <vector>

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

//    std::vector<uint8_t> initData;
    std::unordered_map<Addr, std::vector<uint8_t>> initData;

  public:
    OutgoingRequestBridge(const OutgoingRequestBridgeParams &params);
    ~OutgoingRequestBridge();

    void init();

    AddrRangeList getAddrRanges() const;

    Port & getPort(const std::string &if_name, PortID idx);

//    std::vector<uint8_t> getInitData();
    std::unordered_map<Addr, std::vector<uint8_t>> getInitData();

    void setResponder(SSTResponderInterface* responder);

    bool sendTimingResp(PacketPtr pkt);
    void sendTimingSnoopReq(PacketPtr pkt);

    void handleRecvFunctional(PacketPtr pkt);
};

}; // namespace gem5

#endif //__SST_OUTGOING_REQUEST_BRIDGE_HH__
