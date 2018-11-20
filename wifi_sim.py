from wifi_api import WifiAPI

class WifiSim(WifiAPI):
    uuid = 1

    # mapping from mac to all sims
    Sims = {}

    def __init__(self):
        super(WifiSim, self).__init__(str(WifiSim.uuid))
        WifiSim.uuid += 1

        # mapping from mac to WifiSim
        self.direct_conns = {}

        WifiSim.Sims[self.mac] = self

        self.connected_ap = None

        # list of tuples (mac, strength)
        self.visible_macs = []x

        # queue of messages
        self.data_queue = []

    def prt(self, msg):
        print("Wifi {}: {}".format(self.mac, msg))

    def handle_incoming_data(self, from_mac, data):
        self.prt("from {} got: {}".format(from_mac, data))
        if self.on_recv_handler:
            self.on_recv_handler(from_mac, data)

    def queue_data(self, from_mac, data):
        self.data_queue.append((1, from_mac, data))

    def send_data(self, dest_mac, data):
        if dest_mac in self.direct_conns:
            self.direct_conns[dest_mac].queue_data(self.mac, data)
            return True
        elif dest_mac == self.connected_ap:
            WifiSim.Sims[self.connected_ap].queue_data(self.mac, data)
            return True
        return False

    def is_visible(self, mac):
        for (m, s) in self.visible_macs:
            if m == mac:
                return True

        return False

    def queue_connection(self, mac):
        self.data_queue.append((2, mac))

    def connect_to_ap(self, ap_mac):
        if not self.connected_ap:
            if self.is_visible(ap_mac):
                # self.direct_conns[ap_mac] = WifiSim.Sims[ap_mac]
                self.connected_ap = ap_mac
                WifiSim.Sims[ap_mac].queue_connection(self.mac)
            else:
                self.prt("Cant see {}".format(ap_mac))

    # called when a station connects to this AP
    def station_connection(self, sta_mac):
        self.direct_conns[sta_mac] = WifiSim.Sims[sta_mac]
        # invoke handler
        if self.on_ap_connection_handler:
            self.on_ap_connection_handler(sta_mac)

    def get_direct_connections(self):
        conns = list(self.direct_conns.keys())
        if self.connected_ap:
            conns.append(self.connected_ap)
        return conns

    def queue_disconnection(self, mac):
        self.data_queue.append((3, mac))

    def disconnect_from_ap(self):
        if self.connected_ap:
            # del self.direct_conns[self.connected_ap]
            mac_to_disconnect = self.connected_ap
            self.connected_ap = None
            WifiSim.Sims[mac_to_disconnect].queue_disconnection(self.mac)

    def get_connected_ap(self):
        return self.connected_ap

    def mac_disconnected(self, mac):
        del self.direct_conns[mac]
        if self.on_direct_disconnection_handler:
            self.on_direct_disconnection_handler(mac)

    def get_visible_macs(self):
        return self.visible_macs

    def add_visible_mac(self, mac, strength):
        self.visible_macs.append((mac, strength))

    def remove_visible_mac(self, mac):
        remove_idx = -1
        for i, (vmac, s) in enumerate(self.visible_macs):
            if vmac == mac:
                remove_idx = i
                break
        if remove_idx != -1:
            del self.visible_macs[remove_idx]

    @staticmethod
    def simulate():
        while(True):
            all_empty = True
            for mac in WifiSim.Sims:
                mod = WifiSim.Sims[mac]
                if mod.data_queue:
                    all_empty = False
                    while mod.data_queue:
                        data = mod.data_queue.pop(0)
                        if data[0] == 1:
                            mod.handle_incoming_data(data[1], data[2])
                        elif data[0] == 2:
                            mod.station_connection(data[1])
                        elif data[0] == 3:
                            mod.mac_disconnected(data[1])
            if all_empty:
                break