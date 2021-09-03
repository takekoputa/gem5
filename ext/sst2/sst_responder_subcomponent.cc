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
    this->sst_responder = new SSTResponder(this);
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
SSTResponderSubComponent::handleTimingReq(SST::Interfaces::SimpleMem::Request* request)
{
    this->memory_interface->sendRequest(request);
    return true;
}

void
SSTResponderSubComponent::init(unsigned phase)
{
    if (phase == 0)
    {
        // Get the memory interface
        SST::Params interface_params;
        interface_params.insert("port", "port"); // This is how you tell the interface the name of the port it should use

        // Loads a “memHierarchy.memInterface” into index 0 of the “memory” slot
        // SHARE_PORTS means the interface can use our port as if it were its own
        // INSERT_STATS means the interface will inherit our statistic configuration (e.g., if ours are enabled, the interface’s will be too)
        this->memory_interface = loadAnonymousSubComponent<SST::Interfaces::SimpleMem>(
            "memHierarchy.memInterface", "memory", 0,
            SST::ComponentInfo::SHARE_PORTS | SST::ComponentInfo::INSERT_STATS, interface_params, this->time_converter,
            new SST::Interfaces::SimpleMem::Handler<SSTResponderSubComponent>(this, &SSTResponderSubComponent::portEventHandler)
        );
        assert(this->memory_interface != NULL);
        //SST::MemHierarchy::MemEventInit* mem_event = new SST::MemHierarchy::MemEventInit(this->getName(), SST::MemHierarchy::MemEventInit::InitCommand::Data);
        //this->memory_interface->sendInitData(mem_event);
    }
    else if (phase == 1)
    {
        for (auto p: this->response_receiver->getInitData())
        {
            gem5::Addr addr = p.first;
            std::vector<uint8_t> data = p.second;
            //SST::MemHierarchy::MemEventInit* mem_event = new SST::MemHierarchy::MemEventInit(this->getName(), SST::MemHierarchy::Command::GetX, addr, data);
            //this->memory_link->sendInitData(mem_event);
            SST::Interfaces::SimpleMem::Request* request = new SST::Interfaces::SimpleMem::Request(
                SST::Interfaces::SimpleMem::Request::Command::Write, addr, data.size(), data
            );
            this->memory_interface->sendInitData(request);
        }
    }
    this->memory_interface->init(phase);
}

void
SSTResponderSubComponent::setup()
{
}

void
SSTResponderSubComponent::handleSwapReqFirstStage(gem5::PacketPtr pkt)
{
    // send a read request first
    SST::Interfaces::SimpleMem::Request::Command cmd = SST::Interfaces::SimpleMem::Request::Command::Read;
    SST::Interfaces::SimpleMem::Addr addr = pkt->getAddr();

    uint8_t* data_ptr = pkt->getPtr<uint8_t>();
    auto data_size = pkt->getSize();
    std::vector<uint8_t> data = std::vector<uint8_t>(data_ptr, data_ptr + data_size);

    SST::Interfaces::SimpleMem::Request* request = new SST::Interfaces::SimpleMem::Request(
        cmd, addr, data_size, data
    );

    request->setMemFlags(SST::Interfaces::SimpleMem::Request::Flags::F_LOCKED);
    sst_request_id_to_packet_map[request->id] = pkt;
    this->memory_interface->sendRequest(request);
}

void
SSTResponderSubComponent::handleSwapReqSecondStage(SST::Interfaces::SimpleMem::Request* request)
{
    // get the data, then,
    //     1. send a response to gem5 with the original data
    //     2. send a write with atomic op applied to memory

    SST::Interfaces::SimpleMem::Request::id_t request_id = request->id;
    TPacketMap::iterator it = this->sst_request_id_to_packet_map.find(request_id);
    assert(it != sst_request_id_to_packet_map.end());
    std::vector<uint8_t> data = request->data;

    // step 1
    gem5::PacketPtr pkt = it->second;
    pkt->setData(request->data.data());
    pkt->makeAtomicResponse();
    pkt->headerDelay = pkt->payloadDelay = 0;
    if (this->blocked() || !this->response_receiver->sendTimingResp(pkt))
        this->response_queue.push(pkt);

    // step 2
    (*(pkt->getAtomicOp()))(data.data()); // apply the atomic op
    SST::Interfaces::SimpleMem::Request::Command cmd = SST::Interfaces::SimpleMem::Request::Command::Write;
    SST::Interfaces::SimpleMem::Addr addr = request->addr;
    auto data_size = data.size();
    SST::Interfaces::SimpleMem::Request* write_request = new SST::Interfaces::SimpleMem::Request(
        cmd, addr, data_size, data
    );
    write_request->setMemFlags(SST::Interfaces::SimpleMem::Request::Flags::F_LOCKED);
    this->memory_interface->sendRequest(write_request);

    delete request;
}

void
SSTResponderSubComponent::portEventHandler(SST::Interfaces::SimpleMem::Request* request)
{
    // Expect to handle an SST response
    SST::Interfaces::SimpleMem::Request::id_t request_id = request->id;

    TPacketMap::iterator it = this->sst_request_id_to_packet_map.find(request_id);

    if (it != sst_request_id_to_packet_map.end()) // replying to prior request
    {
        gem5::PacketPtr pkt = it->second; // the packet that needs response

        if ((gem5::MemCmd::Command)pkt->cmd.toInt() == gem5::MemCmd::SwapReq)
        {
            this->handleSwapReqSecondStage(request);
            return;
        }

        sst_request_id_to_packet_map.erase(it);

        Translator::inplaceSSTRequestToGem5PacketPtr(pkt, request);

        if (this->blocked() || !this->response_receiver->sendTimingResp(pkt))
        {
            this->response_queue.push(pkt);
        }
    }
    else // we can handle unexpected invalidates, but nothing else.
    {
        SST::Interfaces::SimpleMem::Request::Command cmd = request->cmd;
        if (cmd == SST::Interfaces::SimpleMem::Request::Command::WriteResp)
            return;
        assert(cmd == SST::Interfaces::SimpleMem::Request::Command::Inv);

        // make Req/Pkt for Snoop/no response needed
        // presently no consideration for masterId, packet type, flags...
        gem5::RequestPtr req = std::make_shared<gem5::Request>(
            request->addr, request->size, 0, 0
        );

        gem5::PacketPtr pkt = new gem5::Packet(req, gem5::MemCmd::InvalidateReq);

        // Clear out bus delay notifications
        pkt->headerDelay = pkt->payloadDelay = 0;

        this->response_receiver->sendTimingSnoopReq(pkt);
    }

    delete request;
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
    /*
    gem5::MemCmd::Command pktCmd = (gem5::MemCmd::Command)pkt->cmd.toInt();
    //assert(pktCmd == gem5::MemCmd::WriteReq);
    if (pktCmd != gem5::MemCmd::WriteReq)
        return;
    gem5::Addr addr = pkt->getAddr();
    // https://stackoverflow.com/questions/9510684/assigning-a-vector-from-an-array-pointer/9510724
    std::vector<uint8_t> data(pkt->getPtr<uint8_t>(), pkt->getPtr<uint8_t>() + pkt->getSize());
    //SST::MemHierarchy::MemEventInit* mem_event = new SST::MemHierarchy::MemEventInit(this->getName(), SST::MemHierarchy::Command::GetX, addr, data);
    //mem_event->setAddr(addr);
    //mem_event->setPayload(pkt->getSize(), pkt->getPtr<uint8_t>());
    SST::Interfaces::SimpleMem::Request* request = new SST::Interfaces::SimpleMem::Request(SST::Interfaces::SimpleMem::Command::Write, addr, data.size(), data);
    this->init_requests.push_back(request);
    */
}

bool
SSTResponderSubComponent::blocked()
{
    return !(this->response_queue.empty());
}
