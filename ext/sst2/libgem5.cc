#include <core/sst_config.h>

#include "gem5.hh"

static
SST::Component* create_gem5(SST::ComponentId_t id, SST::Params &params)
{
    return new gem5Component(id, params);
}