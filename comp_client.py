from comp_wifi import CompWifi
from routing import Routing
from topology import Topology

mesh_id = 1
mesh_mac = "84:f3:eb:59:c2:ca"
# mesh_id = 2
# mesh_mac = "1a:fe:34:a0:75:e1"

w = CompWifi(mesh_id, mesh_mac)
r = Routing(w)
t = Topology(r)

def recv_msg(from_mac, msg):
    print("APP: from {}: {}".format(from_mac, msg))

t.register_recv_msg_handler(recv_msg)

while (True):
    print("APP: Starting cycle")
    w.run()
    t.run()
    print("APP: graph: {}".format(r.get_graph()))