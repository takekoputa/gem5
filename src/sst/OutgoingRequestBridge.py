from m5.params import *
from m5.SimObject import SimObject

class OutgoingRequestBridge(SimObject):
    type = 'OutgoingRequestBridge'
    cxx_header = "sst/outgoing_request_bridge.hh"
    cxx_class = 'gem5::OutgoingRequestBridge'

    port = ResponsePort("Response Port")
