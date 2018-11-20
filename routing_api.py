from abc import ABC, abstractmethod
from recordclass import recordclass

GraphEntry = recordclass("GraphEntry", "seq_num adj_node_dict")

class RoutingAPI(ABC):
    # wifi conforms to wifi_api
    def __init__(self, wifi):
        self.wifi = wifi
        self.mac = wifi.mac

        self.on_recv_msg_handler = None

        wifi.register_recv_handler(self.recv_message)
        wifi.register_direct_disconnection_handler(self.mac_disconnected)
        wifi.register_ap_connection_handler(self.sta_connected)

        # mapping from from mac to (seq_num, neighbor_node_dict)
        # each neighbor node is (to mac, connection type (0 if from is AP and to is STA, 1 else))
        self.graph = {}

    def register_recv_msg_handler(self, handler):
        self.on_recv_msg_handler = handler 

    # called when this node receives a message from a neighboring node
    @abstractmethod
    def recv_message(self, prev_mac, msg):
        pass

    # called when a station disconnects from my access point
    @abstractmethod
    def mac_disconnected(self, mac):
        pass

    # called when a station connects to my access point
    @abstractmethod
    def sta_connected(self, sta_mac):
        pass

    # return all the nodes that this node knows of in the mesh
    @abstractmethod
    def get_graph(self):
        pass

    # send a message through the mesh to the destination node
    @abstractmethod
    def send_message(self, dest_mac, msg):
        pass

    # connect to an ap (must be valid neighbor)
    @abstractmethod
    def connect_to_ap(self, ap_mac):
        pass

    # get a list of macs which are your direct connections
    @abstractmethod
    def get_direct_connections(self):
        pass

    # disconnect from AP you connected to with connect_to_ap
    @abstractmethod
    def disconnect_from_ap(self):
        pass

    @abstractmethod
    def get_connected_ap(self):
        pass

    # returns tuples of (mac, strength)
    @abstractmethod
    def get_visible_macs(self):
        pass

    # adds an edge
    @abstractmethod
    def add_graph_edge(self, ap_mac, sta_mac):
        pass