#include "sst_responder.hh"

#include <cassert>

SSTResponder::SSTResponder(SST::ComponentId_t id, SST::Params& params)
    : SubComponent(id)
{
}

SSTResponder::~SSTResponder()
{
}

void
SSTResponder::init(unsigned phase)
{
}

bool
SSTResponder::findPort(const std::string& port_name)
{
    assert("SSTResponder::findPort not implemented");
}
