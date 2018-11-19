from routing import RoutingLayer

class TestWifi():
    uuid = 1

    Modules = []

    def __init__(self):
        self.mac = str(TestWifi.uuid)
        TestWifi.uuid += 1
        self.Router = RoutingLayer(self)

        # mapping from mac to TestWifi
        self.direct_conns = {}

        self.data_queue = []

        # add to module queue
        TestWifi.Modules.append(self)

    def get_direct_macs(self):
        ret = []
        for conn in self.direct_conns:
            ret.append(self.direct_conns[conn].mac)
        return ret

    def ap_connection(self, sta_mod):
        self.direct_conns[sta_mod.mac] = sta_mod
        self.Router.made_ap_connection(sta_mod.mac)

    def sta_connection(self, ap_mod):
        self.direct_conns[ap_mod.mac] = ap_mod

    def remove_connection(self, testmod):
        del self.direct_conns[testmod.mac]
        self.Router.lost_connection(testmod.mac)

    def queue_data(self, from_mac, data):
        self.data_queue.append((from_mac, data))

    def send_data_to_mac(self, dest_mac, msg):
        if dest_mac in self.direct_conns:
            self.direct_conns[dest_mac].queue_data(self.mac, msg)
            return
        # tell the router we no longer have a direct connection to this mac
        self.Router.lost_connection(dest_mac)

    def prt(self, msg):
        print("{}: {}".format(self.mac, msg))

    def handle_data(self, prev_mac, data):
        self.prt("got: {}".format(data))
        packets = data.split(",")
        if len(packets) < 1:
            return
        self.Router.process_data(prev_mac, packets)

def simulate():
    while(True):
        all_empty = True
        for mod in TestWifi.Modules:
            if mod.data_queue:
                all_empty = False
                while mod.data_queue:
                    data = mod.data_queue.pop(0)
                    mod.handle_data(data[0], data[1])
        if all_empty:
            break

def make_connection(ap, sta):
    # print("making ap connection from {} to {}".format(ap.mac, sta.mac))
    ap.ap_connection(sta)
    # print("making ap connection from {} to {}".format(sta.mac, ap.mac))
    sta.sta_connection(ap)

def remove_connection(m1, m2):
    m1.remove_connection(m2)
    m2.remove_connection(m1)

m1 = TestWifi()
m2 = TestWifi()
m3 = TestWifi()
m4 = TestWifi()
m5 = TestWifi()
m6 = TestWifi()
make_connection(m1,m2)
simulate()
make_connection(m2,m3)
simulate()
make_connection(m3,m4)
simulate()
make_connection(m2,m5)
simulate()
make_connection(m5,m6)
simulate()
make_connection(m6,m4)
simulate()
m1.Router.create_msg(m4.mac, "Hi")
simulate()
m4.Router.create_msg(m1.mac, "Hi Back")
simulate()
m6.Router.create_msg(m1.mac, "Hi from me")
simulate()
remove_connection(m2, m3)
# simulate()
# now the path to 4 is through 2->5->6->4
m1.Router.create_msg(m4.mac, "Hi again")
simulate()
m3.Router.create_msg(m1.mac, "Hi from 3")
simulate()