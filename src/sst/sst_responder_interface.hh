#ifndef __SST_RESPONDER_INTERFACE_HH__
#define __SST_RESPONDER_INTERFACE_HH__

#include "mem/port.hh"

namespace gem5
{
class SSTResponderInterface
{
  public:
    SSTResponderInterface();
    virtual bool handleTimingReq(PacketPtr pkt) = 0;
};
} // namespace gem5

#endif // __SST_RESPONDER_INTERFACE_HH__
