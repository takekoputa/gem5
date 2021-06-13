from m5.params import *
from m5.SimObject import SimObject

class OutgoingRequestBridge(SimObject):
    type = 'OutgoingRequestBridge'
    cxx_header = "sst/outgoing_request_bridge.hh"

    port = ResponsePort("Response Port")
