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

#include <sim/sim_object.hh>
#include <sst/outgoing_request_bridge.hh>

//#ifdef fatal
//#undef fatal
//#endif

#include <sst/core/eli/elementinfo.h>
#include <sst/core/link.h>

class SSTResponder: public SST::Component
{
  public:
    SSTResponder(SST::ComponentId_t id, SST::Params& params);
    ~SSTResponder();

    OutgoingRequestBridge* response_receiver;
    SST::Link* memory_link; // sending requests to SST::Memory
                            // receive responses from SST::Memory
    bool findPort(const std::string& port_name);

  public: // register the component to SST
    SST_ELI_REGISTER_COMPONENT(
        SSTResponder,
        "gem5", // SST will look for libgem5.so
        "gem5Bridge",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Initialize gem5 and link SST's ports to gem5's ports",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"receiver_name", "Name of the SimObject receiving the responses"}
    )

};

#endif // __SST_RESPONDER_HH__
