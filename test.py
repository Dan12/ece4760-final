from routing import Routing
from wifi_sim import WifiSim
from topology import Topology

class App():
    def __init__(self):
        self.w = WifiSim()
        self.r = Routing(self.w)
        self.t = Topology(self.r)
        self.t.register_recv_msg_handler(self.recv_msg)
        self.mac = self.t.mac

    def put_in_range(self, other, s):
        self.w.add_visible_mac(other.mac, s)
        other.w.add_visible_mac(self.mac, s)

        Topology.simulate()
        WifiSim.simulate()
        Topology.simulate()
        WifiSim.simulate()

    def put_in_range_seq(self, other, s):
        self.w.add_visible_mac(other.mac, s)

        Topology.simulate()
        WifiSim.simulate()
        other.w.add_visible_mac(self.mac, s)
        Topology.simulate()
        WifiSim.simulate()
        Topology.simulate()
        WifiSim.simulate()

    def take_out_of_range(self, other):
        self.w.remove_visible_mac(other.mac)
        other.w.remove_visible_mac(self.mac)
        self.w.mac_disconnected(other.mac)
        other.w.mac_disconnected(self.mac)

        Topology.simulate()
        WifiSim.simulate()
        Topology.simulate()
        WifiSim.simulate()

    def connect(self, other):
        self.w.add_visible_mac(other.mac, 1)
        other.w.add_visible_mac(self.mac, 1)
        self.w.connect_to_ap(other.mac)
        WifiSim.simulate()

    def disconnect(self, other):
        self.w.remove_visible_mac(other.mac)
        other.w.remove_visible_mac(self.mac)
        self.w.mac_disconnected(other.mac)
        other.w.mac_disconnected(self.mac)
        WifiSim.simulate()

    def send_message(self, mac, msg):
        self.t.send_msg(mac, msg)
        WifiSim.simulate()

    def recv_msg(self, from_mac, msg):
        print("APP {}: MSG from {}: {}".format(self.mac, from_mac, msg))

    def is_consistent_graph(self):
        print("Consitent for {}?".format(self.mac))
        graph = self.t.router.get_graph()
        for t in Topology.Sims:
            tg = t.router.get_graph()
            for node in tg:
                for elt in tg[node].adj_node_dict:
                    if node in graph:
                        if elt in graph[node].adj_node_dict:
                            if tg[node].adj_node_dict[elt] != graph[node].adj_node_dict[elt]:
                                print("{}: {}[{}] expected {} got {}".format(t.mac, node, elt, graph[node].adj_node_dict[elt], tg[node].adj_node_dict[elt]))        
                                break
                        else:
                            print("{}: elt {} not node's {} list".format(t.mac, elt, node))    
                            break
                    else:
                        print("{}: Node {} not in graph".format(t.mac, node))
                        break
        print("Finished consistent for {}?".format(self.mac))

a1 = App()
a2 = App()
a3 = App()
a4 = App()

# Basic routing test (manual connections)
# a1.connect(a2)
# a3.connect(a2)
# a4.connect(a2)

# a2.send_message(a1.mac, "Hi from 2")
# a1.send_message(a2.mac, "Hi back from 1")
# a1.send_message(a3.mac, "Hi to 3 from 1")
# a4.send_message(a1.mac, "Hi to 1 from 4")

# a1.disconnect(a2)
# a1.connect(a4)
# a1.send_message(a3.mac, "Hi to 3 from 1-2")
# a2.connect(a1)
# a1.send_message(a3.mac, "Hi to 3 from 1-3")
# a2.disconnect(a1)
# a4.disconnect(a2)
# a1.send_message(a3.mac, "Hi to 3 from 1-4")
# a4.connect(a3)
# a1.send_message(a3.mac, "Hi to 3 from 1-5")
# a1.send_message(a2.mac, "Hi to 2 from 1-2")

# a1.is_consistent_graph()
# a2.is_consistent_graph()
# a3.is_consistent_graph()
# a4.is_consistent_graph()
# print(a1.r.get_graph())
# print(a2.r.get_graph())

# Auto connection and Sparse connection test
# a2.put_in_range(a1, 5)
# a3.put_in_range(a1, 7)
# a4.put_in_range(a1, 5)

# a4.put_in_range_seq(a2, 10)

# a1.is_consistent_graph()
# a2.is_consistent_graph()
# a3.is_consistent_graph()
# a4.is_consistent_graph()
# print(a1.r.get_graph())

# a2.send_message(a1.mac, "Hi from 2")
# a1.send_message(a2.mac, "Hi back from 1")
# a1.send_message(a3.mac, "Hi to 3 from 1")
# a4.send_message(a3.mac, "Hi to 3 from 4")


# reverse edge test
a2.put_in_range_seq(a1, 5)
a3.put_in_range_seq(a4, 5)

a2.send_message(a1.mac, "Hi from 2")
a1.send_message(a2.mac, "Hi back from 1")
a1.send_message(a3.mac, "Hi to 3 from 1")

a3.put_in_range(a2, 7)

a1.send_message(a3.mac, "Hi to 3 from 1")
a1.send_message(a4.mac, "Hi to 4 from 1-1")

a1.is_consistent_graph()
a2.is_consistent_graph()
a3.is_consistent_graph()
a4.is_consistent_graph()
print(a1.r.get_graph())

a3.take_out_of_range(a2)
a1.send_message(a4.mac, "Hi to 4 from 1-2")

a3.put_in_range(a2, 7)
a1.send_message(a4.mac, "Hi to 4 from 1-3")
a4.send_message(a1.mac, "Hi to 1 from 4-3")

a1.is_consistent_graph()
a2.is_consistent_graph()
a3.is_consistent_graph()
a4.is_consistent_graph()
print(a1.r.get_graph())