#ifndef __SST_OUTGOING_REQUEST_BRIDGE_HH__
#define __SST_OUTGOING_REQUEST_BRIDGE_HH__

#include "mem/external_slave.hh"
#include "mem/port.hh"
#include "params/OutgoingRequestBridge.hh"
#include "sim/sim_object.hh"

class OutgoingRequestBridge;

class OutgoingRequestBridge: public SimObject
{
  public:
    class OutgoingRequestPort: public ResponsePort
    {
      public:
        OutgoingRequestPort(const std::string &name_,
                            OutgoingRequestBridge* owner);
        ~OutgoingRequestPort();
        Tick recvAtomic(PacketPtr pkt);
        void recvFunctional(PacketPtr pkt);
        bool recvTimingReq(PacketPtr pkt);
        void recvRespRetry();
        AddrRangeList getAddrRanges() const;
    };

  public:
    OutgoingRequestPort outgoingPort;

  public:
    OutgoingRequestBridge(const OutgoingRequestBridgeParams& params);
    ~OutgoingRequestBridge();

    Port & getPort(const std::string &if_name, PortID idx);

    void callback_when_received();
};

#endif //__SST_OUTGOING_REQUEST_BRIDGE_HH__
