#include "sst_responder_subcomponent.hh"

#include <cassert>

SSTResponderSubComponent::SSTResponderSubComponent(SST::ComponentId_t id, SST::Params& params)
    : SubComponent(id)
{
    //this->sst_responder = new SSTResponder(this);
}

SSTResponderSubComponent::~SSTResponderSubComponent()
{
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

bool
SSTResponderSubComponent::handleTimingReq(SST::MemHierarchy::MemEvent* mem_event)
{
    this->memory_link->send(mem_event);
    return true;
}

void
SSTResponderSubComponent::init(unsigned phase)
{
}

void
SSTResponderSubComponent::setup(SST::TimeConverter* tc)
{
    //this->memory_link = this->configureLink("cpu_l1_cache_link");
    this->memory_link = this->configureLink("port", tc);
    assert(this->memory_link != NULL);
}

