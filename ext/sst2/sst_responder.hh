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

#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/memTypes.h>
#include <sst/elements/memHierarchy/util.h>

// from gem5
#include <sim/sim_object.hh>
#include <sst/outgoing_request_bridge.hh>
#include <sst/sst_responder_interface.hh>

#include "sst_responder_subcomponent.hh"

class SSTResponderSubComponent;

class SSTResponder: public gem5::SSTResponderInterface
{
  private:
    SSTResponderSubComponent* owner;
    SST::Output* output;
  public:
    SSTResponder(SSTResponderSubComponent* owner_);
    ~SSTResponder();

    void setOutputStream(SST::Output* output_);

    bool handleTimingReq(gem5::PacketPtr pkt) override;

    std::string getName();
};

#endif // __SST_RESPONDER_HH__
