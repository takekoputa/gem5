#include "sst_responder_subcomponent.hh"

#include <cassert>

#ifdef fatal  // gem5 sets this
#undef fatal
#endif

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
SSTResponderSubComponent::setTimeConverter(SST::TimeConverter* tc)
{
    this->time_converter = tc;
}

void
SSTResponderSubComponent::setOutputStream(SST::Output* output_)
{
    this->output = output_;
}

void
SSTResponderSubComponent::setResponseReceiver(gem5::OutgoingRequestBridge* gem5_bridge)
{
    this->response_receiver = gem5_bridge;
    this->response_receiver->setResponder(this->sst_responder);
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
    if (phase == 0)
    {
        this->sst_responder = new SSTResponder(this);
    }
    else if (phase == 1)
    {
        //this->memory_link = this->configureLink("cpu_l1_cache_link");
        this->memory_link = this->configureLink(
            "port", this->time_converter,
            new SST::Event::Handler<SSTResponderSubComponent>(this, &SSTResponderSubComponent::portEventHandler));
        assert(this->memory_link != NULL);
        SST::MemHierarchy::MemEventInit* mem_event = new SST::MemHierarchy::MemEventInit(this->getName(), SST::MemHierarchy::MemEventInit::InitCommand::Data);
        this->memory_link->sendInitData(mem_event);
    }
}

void
SSTResponderSubComponent::portEventHandler(SST::Event* ev)
{
    //assert(false && "portEventHandler is not implemented");
}

void
SSTResponderSubComponent::setup(SST::TimeConverter* tc)
{
    //this->memory_link = this->configureLink("cpu_l1_cache_link");
    //this->memory_link = this->configureLink(
    //    "port", tc,
    //    new SST::Event::Handler<SSTResponderSubComponent>(this, &SSTResponderSubComponent::portEventHandler));
    //assert(this->memory_link != NULL);
    //std::vector<uint8_t> data;
    //SST::MemHierarchy::MemEvent* mem_event = new SST::MemHierarchy::MemEvent(this->getName(), 0, 0, SST::MemHierarchy::Command::NULLCMD);
    //mem_event->setPayload(data);
    //this->memory_link->sendInitData(mem_event);
}

