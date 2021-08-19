#ifndef __SST_RESPONDER_HH__
#define __SST_RESPONDER_HH__

#define TRACING_ON 0

#include <string>
#include <vector>

#include <sst/core/sst_config.h>
#include <sst/core/component.h>

#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/interfaces/simpleMem.h>

#include <sst/core/eli/elementinfo.h>
#include <sst/core/link.h>

// from gem5
#include <sim/sim_object.hh>
#include <sst/outgoing_request_bridge.hh>
#include <sst/sst_responder_interface.hh>

class SSTResponder: public gem5::SSTResponderInterface
{
  private:
    SST::SubComponent* owner;
  public:
    SSTResponder(SST::SubComponent* owner_);
    ~SSTResponder();

    bool handleTimingReq(gem5::PacketPtr pkt) override;

};

#endif // __SST_RESPONDER_HH__
