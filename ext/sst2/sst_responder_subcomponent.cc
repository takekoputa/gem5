#include "sst_responder_subcomponent.hh"

#include <cassert>
#include <sstream>
#include <iomanip>

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
    //auto init_packets = gem5_bridge->getInitPackets();
    //for (auto pkt: init_packets)
    //{
    //    this->handleRecvFunctional(pkt);
    //}
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
    else if (phase == 2)
    {
        /*
        for (auto pkt: this->response_receiver->getInitPackets())
        {
            gem5::MemCmd::Command pktCmd = (gem5::MemCmd::Command)pkt->cmd.toInt();
            //assert(pktCmd == gem5::MemCmd::WriteReq);
            if (pktCmd != gem5::MemCmd::WriteReq)
                continue;
            gem5::Addr addr = pkt->getAddr();
            // https://stackoverflow.com/questions/9510684/assigning-a-vector-from-an-array-pointer/9510724
//            std::vector<uint8_t> data(pkt->getPtr<uint8_t>(), pkt->getPtr<uint8_t>() + pkt->getSize());
            std::vector<uint8_t> data;
            auto ptr = pkt->getPtr<uint8_t>();
            for (unsigned int k = 0; k < pkt->getSize(); k++)
            {
                data.push_back(*ptr);
                ptr++;
            }
            if (addr >= 0x87e00000)
            {
                std::stringstream s;
                s << this->getName() << " sends "<< pkt->getSize() << " packets to [";
                for (auto n: data)
                    s << std::hex << (uint16_t)n << ",";
                s << "] to " << std::hex << addr;
                this->output->output(CALL_INFO, "%s\n", s.str().c_str());
            }
            SST::MemHierarchy::MemEventInit* mem_event = new SST::MemHierarchy::MemEventInit(this->getName(), SST::MemHierarchy::Command::GetX, addr, data);
            this->memory_link->sendInitData(mem_event);
        }
        */
        for (auto p: this->response_receiver->getInitData())
        {
            gem5::Addr addr = p.first;
            std::vector<uint8_t> data = p.second;
            SST::MemHierarchy::MemEventInit* mem_event = new SST::MemHierarchy::MemEventInit(this->getName(), SST::MemHierarchy::Command::GetX, addr, data);
            this->memory_link->sendInitData(mem_event);
        }
        
    }
}

void
SSTResponderSubComponent::setup()
{
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

void
SSTResponderSubComponent::handleRecvFunctional(gem5::PacketPtr pkt)
{
    gem5::MemCmd::Command pktCmd = (gem5::MemCmd::Command)pkt->cmd.toInt();
    //assert(pktCmd == gem5::MemCmd::WriteReq);
    if (pktCmd != gem5::MemCmd::WriteReq)
        return;
    gem5::Addr addr = pkt->getAddr();
    // https://stackoverflow.com/questions/9510684/assigning-a-vector-from-an-array-pointer/9510724
    std::vector<uint8_t> data(pkt->getPtr<uint8_t>(), pkt->getPtr<uint8_t>() + pkt->getSize());
    SST::MemHierarchy::MemEventInit* mem_event = new SST::MemHierarchy::MemEventInit(this->getName(), SST::MemHierarchy::Command::GetX, addr, data);
    //mem_event->setAddr(addr);
    //mem_event->setPayload(pkt->getSize(), pkt->getPtr<uint8_t>());
    this->init_events.push_back(mem_event);
}

bool
SSTResponderSubComponent::blocked()
{
    return !(this->response_queue.empty());
}
