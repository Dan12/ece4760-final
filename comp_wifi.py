from wifi_api import WifiAPI
import socket
import uuid
import re
import time
import datetime

def int32(x):
  if x>0xFFFFFFFF:
    raise OverflowError
  if x>0x7FFFFFFF:
    x=int(0x100000000-x)
    if x<2147483648:
      return -x
    else:
      return -2147483648
  return x

def parse_mac(mac):
    p = mac.split(":")
    m = int(p[2], 16) << 24
    m |= int(p[3], 16) << 16 
    m |= int(p[4], 16) << 8
    m |= int(p[5], 16)
    return str(int32(m))

class CompWifi(WifiAPI):
    def __init__(self, ap_id, ap_mac):
        super(CompWifi, self).__init__(ap_mac)
        self.client = None
        self.vis_mac = ap_mac
        self.ap_mac = None
        self.ap_id = ap_id
        self.to_ping = 0
        self.mac = parse_mac(':'.join(re.findall('..', '%012x' % uuid.getnode())))

    def prt(self, msg):
        print("C-WIFI: {}".format(msg))

    def send_over_socket(self, dest_mac, data):
        if dest_mac == self.ap_mac and self.client:
            self.client.send(data.encode())

    def send_data(self, dest_mac, data):
        self.send_over_socket(dest_mac, "M,{}\n".format(data))

    def connect_to_ap(self, ap_mac):
        self.ap_mac = ap_mac
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        self.client.connect(("192.168.{}.1".format(self.ap_id), 80))


        # send station connection confirmation
        self.send_over_socket(ap_mac, "CS,{}\n".format(self.mac))

    def get_direct_connections(self):
        return [self.ap_mac]

    def disconnect_from_ap(self):
        pass

    def get_connected_ap(self):
        return self.ap_mac

    def get_visible_macs(self):
        return [(self.vis_mac, 1)]

    def ping(self):
        self.send_over_socket(self.ap_mac, "P,{}\n".format(5000))

    def run(self):
        if self.client:
            if self.to_ping != 0 and self.to_ping < datetime.datetime.now().timestamp():
                self.to_ping = 0
                self.ping()

            self.client.settimeout(5.0)
            try:
                data = self.client.recv(4096).decode('utf-8', errors='ignore')
            except:
                return
            self.prt(data)
            packets = data[:-1].split(",")
            self.prt("RECV packets: {}".format(packets))
            if len(packets) < 1:
                return

            # ping packet to keep connection alive
            if packets[0] == "P":
                self.to_ping = (datetime.datetime.now().timestamp()+int(packets[1])/1000)
            elif packets[0] == "M":
                if self.on_recv_handler:
                    self.on_recv_handler(self.ap_mac, ",".join(packets[1:]))