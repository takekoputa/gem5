#include "sst_responder_subcomponent.hh"

#include <cassert>

SSTResponderSubComponent::SSTResponderSubComponent(SST::ComponentId_t id, SST::Params& params)
    : SubComponent(id)
{
    //this->sst_responder = new SSTResponder(this);
}

SSTResponderSubComponent::~SSTResponderSubComponent()
{
    assert(false);
    delete this->sst_responder;
}

void
SSTResponderSubComponent::setResponseReceiver(gem5::OutgoingRequestBridge* gem5_bridge)
{
    this->response_receiver = gem5_bridge;
    this->sst_responder = new SSTResponder(this);
    this->response_receiver->setResponder(this->sst_responder);
    //gem5_bridge->sstResponder = this->sst_responder;
    //assert(false);
}

void
SSTResponderSubComponent::init(unsigned phase)
{
}

void
SSTResponderSubComponent::setup()
{
    //this->sst_responder = new SSTResponder(this);
    //this->response_receiver->setResponder(this->sst_responder);
}

