import glob
from wifi_real import WifiReal
from routing import Routing
from topology import Topology
import threading
import time
from random import randint

class TestWifi(threading.Thread):
    def __init__(self, port, id):
        threading.Thread.__init__(self)
        self.id = id
        self.w = WifiReal(port,id)
        self.r = Routing(self.w)
        self.t = Topology(self.r)

    def prt(self, msg):
        print("APP {}: {}".format(self.id, msg))

    def run(self):
        # print(self.module.get_visible_macs())
        while(True):
            self.prt("Starting cycle")
            self.w.run()
            # self.t.run()
            self.prt(self.r.get_graph())
            s_time = time.time()
            while(time.time() - s_time < 5 + (randint(0, 20)/5)):
                self.w.listen_for_lines()

# id = 2
# for port in glob.glob("/dev/cu.SLAB_USBtoUART*"):
#     if (port != "/dev/cu.SLAB_USBtoUART"):
#         TestWifi(port, id).start()
#         id+=1

TestWifi("COM4", 2).start()
