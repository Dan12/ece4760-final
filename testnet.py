from routing import RoutingLayer

class TestWifi():
    uuid = 1

    def __init__(self):
        self.mac = TestWifi.uuid
        TestWifi.uuid += 1
        self.Router = RoutingLayer(self)

        # mapping from mac to TestWifi
        self.direct_conns = {}

    def get_direct_macs(self):
        ret = []
        for conn in self.direct_conns:
            ret.append(conn.mac)
        return ret

    def add_connection(self, testmod):
        self.direct_conns[testmod.mac] = testmod

    def remove_connection(self, testmod):
        del self.direct_conns[testmod.mac]

    def send_data_to_mac(self, dest_mac, msg):
        if dest_mac in self.direct_conns:
            self.direct_conns[dest_mac].handle_data(self.mac, msg)

    def handle_data(self, prev_mac, data):
        packets = data.split(",")
        if len(packets) < 1:
            return
        self.Router.process_data(prev_mac, packets)

m1 = TestWifi()
m2 = TestWifi()
m1.add_connection(m2)
m2.add_connection(m1)
m1.Router.create_msg(m2.mac, "Hi")
