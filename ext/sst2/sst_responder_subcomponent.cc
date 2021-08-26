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
    // Expect to handle sending packets from SST to gem5
    SST::MemHierarchy::MemEvent* mem_event = dynamic_cast<SST::MemHierarchy::MemEvent*>(ev);
    if (!mem_event)
    {
        output->fatal(CALL_INFO, 1, "SSTRes457fponder received a non-MemEvent Event\n");
        delete ev;
        return;
    }
    SST::Event::id_type event_id = mem_event->getID();

    TPacketMap::iterator it = event_id_to_packet_map.find(event_id);

    if (it != event_id_to_packet_map.end()) // replying to prior request
    {
        gem5::PacketPtr pkt = it->second; // the packet that needs response
        event_id_to_packet_map.erase(it);

        Translator::inplaceSSTMemEventToGem5PacketPtr(pkt, mem_event);

        if (this->blocked() || !this->response_receiver->sendTimingResp(pkt))
        {
            this->response_queue.push(pkt);
        }
    }
    else // we can handle unexpected invalidates, but nothing else.
    {
        SST::MemHierarchy::Command cmd = mem_event->getCmd();
        assert(cmd == SST::MemHierarchy::Command::Inv);

        // make Req/Pkt for Snoop/no response needed
        // presently no consideration for masterId, packet type, flags...
        gem5::RequestPtr req = std::make_shared<gem5::Request>(
            mem_event->getAddr(), mem_event->getSize(), 0, 0);

        gem5::PacketPtr pkt = new gem5::Packet(req, gem5::MemCmd::InvalidateReq);

        // Clear out bus delay notifications
        pkt->headerDelay = pkt->payloadDelay = 0;

        this->response_receiver->sendTimingSnoopReq(pkt);
    }

    delete mem_event;
}

void
SSTResponderSubComponent::handleRecvRespRetry()
{
    while (this->blocked() && this->response_receiver->sendTimingResp(this->response_queue.front()))
    {
        this->response_queue.pop();
    }
}

bool
SSTResponderSubComponent::blocked()
{
    return !(this->response_queue.empty());
}