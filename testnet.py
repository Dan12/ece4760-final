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

    def add_connection(self, testmod):
        self.direct_conns[testmod.mac] = testmod

    def remove_connection(self, testmod):
        del self.direct_conns[testmod.mac]

    def queue_data(self, from_mac, data):
        self.data_queue.append((from_mac, data))

    def send_data_to_mac(self, dest_mac, msg):
        if dest_mac in self.direct_conns:
            self.direct_conns[dest_mac].queue_data(self.mac, msg)
            return True
        return False

    def prt(self, msg):
        print("{}: {}".format(self.mac, msg))

    def handle_data(self, prev_mac, data):
        self.prt("got: {}".format(data))
        packets = data.split(",")
        if len(packets) < 1:
            return
        self.Router.process_data(prev_mac, packets)

def simualte():
    while(True):
        all_empty = True
        for mod in TestWifi.Modules:
            if mod.data_queue:
                all_empty = False
                while mod.data_queue:
                    data = mod.data_queue.pop()
                    mod.handle_data(data[0], data[1])
        if all_empty:
            break

def make_connection(m1, m2):
    m1.add_connection(m2)
    m2.add_connection(m1)

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
make_connection(m2,m3)
make_connection(m3,m4)
make_connection(m2,m5)
make_connection(m5,m6)
m1.Router.create_msg(m4.mac, "Hi")
simualte()
m4.Router.create_msg(m1.mac, "Hi Back")
simualte()
