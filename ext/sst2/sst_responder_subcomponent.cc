#include "sst_responder_subcomponent.hh"

#include <cassert>

SSTResponderSubComponent::SSTResponderSubComponent(SST::ComponentId_t id, SST::Params& params)
    : SubComponent(id)
{
    //this->sst_responder = new SSTResponder(this);
}

SSTResponderSubComponent::~SSTResponderSubComponent()
{
    //delete this->sst_responder;
}

void
SSTResponderSubComponent::setResponseReceiver(gem5::OutgoingRequestBridge* gem5_bridge)
{
    this->response_receiver = gem5_bridge;
    gem5_bridge->sstResponder = new SSTResponder(this);
    //this->sst_responder = gem5_bridge->sstResponder;
}

void
SSTResponderSubComponent::init(unsigned phase)
{
}

