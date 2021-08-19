#ifndef __SST_RESPONDER_SUBCOMPONENT_HH__
#define __SST_RESPONDER_SUBCOMPONENT_HH__

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

#include "sst_responder.hh"

class SSTResponderSubComponent: public SST::SubComponent
{
  private:
    gem5::OutgoingRequestBridge* response_receiver;
    SST::Link* memory_link; // sending requests to SST::Memory
                            // receive responses from SST::Memory
    SSTResponder* sst_responder;

  public:
    SSTResponderSubComponent(SST::ComponentId_t id, SST::Params& params);
    ~SSTResponderSubComponent();

    void init(unsigned phase);

    void setResponseReceiver(gem5::OutgoingRequestBridge* gem5_bridge);

  public: // register the component to SST
    SST_ELI_REGISTER_SUBCOMPONENT_API(SSTResponderSubComponent);
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        SSTResponderSubComponent,
        "gem5", // SST will look for libgem5.so
        "gem5Bridge",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Initialize gem5 and link SST's ports to gem5's ports",
        SSTResponderSubComponent
    )

    SST_ELI_DOCUMENT_PORTS(
        {"port", "Handling mem events", {"memHierarchy.MemEvent",""}}
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"receiver_name", "Name of the SimObject receiving the responses"}
    )

};

#endif // __SST_RESPONDER_SUBCOMPONENT_HH__
