from comp_wifi import CompWifi
from routing import Routing
from topology import Topology
import threading
import time
from graph import Graph

get_ipt = True

if get_ipt:
    mesh_id = 225
    mesh_mac = "882931169"
# mesh_id = 202
# mesh_mac = "-346438966"
else:
    mesh_id = 79
    mesh_mac = "-346438577"

w = CompWifi(mesh_id, mesh_mac)
r = Routing(w)
t = Topology(r)

def recv_msg(from_mac, msg):
    print("APP: from {}: {}".format(from_mac, msg))

t.register_recv_msg_handler(recv_msg)

class GetIpt(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.ipt = None

    def run(self):
        while(True):
            self.ipt = input()
            print("Got input: {}".format(self.ipt))

if not get_ipt:
    g = Graph()
    g.start()
else:
    i = GetIpt()
    i.start()
while (True):
    print("APP: Starting cycle")
    w.run()
    t.run()
    print("APP: graph: {}".format(r.get_graph()))

    if not get_ipt:
        g.set_graph(r.get_graph())
    else:
        if i.ipt:
            p = i.ipt.split(",")
            i.ipt = None
            if len(p) == 2:
                t.send_msg(p[0], p[1])