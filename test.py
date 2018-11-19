from routing_v2 import Routing
from wifi_sim import WifiSim

class App():
    def __init__(self):
        self.w = WifiSim()
        self.r = Routing(self.w)
        self.r.register_recv_msg_handler(self.recv_msg)
        self.mac = self.r.mac

    def put_in_range(self, other, s):
        self.w.add_visible_mac(other.mac, s)
        other.w.add_visible_mac(self.mac, s)

        self.r.tick_topology_algo()
        WifiSim.simulate()
        other.r.tick_topology_algo()
        WifiSim.simulate()

    def send_message(self, mac, msg):
        self.r.send_message(mac, msg)
        WifiSim.simulate()

    def recv_msg(self, from_mac, msg):
        self.w.prt("MSG from {}: {}".format(from_mac, msg))

a1 = App()
a2 = App()
a3 = App()
a4 = App()

# Sparse connection test
# a1.put_in_range(a2, 5)
# a3.put_in_range(a2, 7)
# a4.put_in_range(a2, 5)

# a2.send_message(a1.mac, "Hi from 2")
# a1.send_message(a2.mac, "Hi back from 1")
# a1.send_message(a3.mac, "Hi to 3 from 1")
# a4.send_message(a1.mac, "Hi to 1 from 4")

# a4.put_in_range(a1, 10)

# a4.send_message(a3.mac, "Hi to 3 from 4")

# reverse edge test
a2.put_in_range(a1, 5)
a3.put_in_range(a4, 5)

a2.send_message(a1.mac, "Hi from 2")
a1.send_message(a2.mac, "Hi back from 1")
a1.send_message(a3.mac, "Hi to 3 from 1")

a3.put_in_range(a2, 7)

a1.send_message(a3.mac, "Hi to 3 from 1")

# a4.put_in_range(a2, 5)