from abc import ABC, abstractmethod

class RoutingAPI(ABC):
    # wifi conforms to wifi_api
    def __init__(self, wifi):
        self.wifi = wifi
        self.mac = wifi.mac

        self.on_recv_msg_handler = None

        wifi.register_recv_handler(self.recv_message)
        wifi.register_direct_disconnection_handler(self.sta_disconnected)
        wifi.register_ap_connection_handler(self.sta_connected)

    def register_recv_msg_handler(self, handler):
        self.on_recv_msg_handler = handler 

    # called when this node receives a message from a neighboring node
    @abstractmethod
    def recv_message(self, prev_mac, msg):
        pass

    # called when a station disconnects from my access point
    @abstractmethod
    def sta_disconnected(self, sta_mac):
        pass

    # called when a station connects to my access point
    @abstractmethod
    def sta_connected(self, sta_mac):
        pass

    # return all the nodes that this node knows of in the mesh
    @abstractmethod
    def get_nodes(self):
        pass

    # send a message through the mesh to the destination node
    @abstractmethod
    def send_message(self, dest_mac, msg):
        pass