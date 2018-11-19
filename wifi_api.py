from abc import ABC, abstractmethod

class WifiAPI(ABC):
    def __init__(self, mac):
        self.mac = mac

        self.on_recv_handler = None
        self.on_direct_disconnection_handler = None
        self.on_ap_connection_handler = None

    # handler takes in source mac (neighbor) and message
    def register_recv_handler(self, handler):
        self.on_recv_handler = handler

    # handler takes in station make that disconnected from this ap
    def register_direct_disconnection_handler(self, handler):
        self.on_direct_disconnection_handler = handler

    # handler takes in station mac that connected to this ap
    def register_ap_connection_handler(self, handler):
        self.on_ap_connection_handler = handler

    # send data to the mac address (must be valid neighbor)
    # return True if success, False if failure
    @abstractmethod
    def send_data(self, dest_mac, data):
        pass

    # send data to the mac address (must be valid neighbor)
    @abstractmethod
    def connect_to_ap(self, ap_mac):
        pass

    # get a list of macs which are your direct connections
    @abstractmethod
    def get_direct_connections(self):
        pass

    # disconnect from AP you connected tp with connect_to_ap
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