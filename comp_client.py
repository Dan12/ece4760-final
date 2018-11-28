from comp_wifi import CompWifi
from routing import Routing
from topology import Topology

mesh_id = 225
mesh_mac = "882931169"
# mesh_id = 202
# mesh_mac = "-346438966"

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