from abc import ABC, abstractmethod

class TopologyAPI(ABC):
    def __init__(self, router):
        # conforms to routing api
        self.router = router
        self.mac = router.mac
        router.register_recv_msg_handler(self.recv_msg)

        self.on_recv_msg_handler = None

    def register_recv_msg_handler(self, handler):
        self.on_recv_msg_handler = handler 

    # called when a message is passed from routing to topology layer
    @abstractmethod
    def recv_msg(self, from_mac, msg):
        pass

    # send a message to the destination mac
    @abstractmethod
    def send_msg(self, dest_mac, msg):
        pass

    # get the nodes in the mesh network (list of mac addresses)
    @abstractmethod
    def get_nodes(self):
        pass

    # run the topology algorithms
    @abstractmethod
    def run(self):
        pass