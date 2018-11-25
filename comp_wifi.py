from wifi_api import WifiAPI
import socket
import uuid
import re
import time
import datetime

class CompWifi(WifiAPI):
    def __init__(self, ap_mac, ap_id):
        super(WifiReal, self).__init__(ap_mac)
        self.client = None
        self.ap_mac = ap_mac
        self.ap_id = ap_id
        self.to_ping = 0

    def prt(self, msg):
        print("C-WIFI: {}".format(msg))

    def send_data(self, dest_mac, data):
        if dest_mac == self.ap_mac and self.client:
            self.client.send(data.encode())

    def connect_to_ap(self, ap_mac):
        self.ap_mac = ap_mac
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect(("192.168.{}.1".format(self.ap_id), 80))

        my_mac = ':'.join(re.findall('..', '%012x' % uuid.getnode()))
        # send station connection confirmation
        self.send_data(ap_mac, "CS,{}".format(my_mac))

    def get_direct_connections(self):
        return [self.ap_mac]

    def disconnect_from_ap(self):
        pass

    def get_connected_ap(self):
        return self.ap_mac

    def get_visible_macs(self):
        return [(self.ap_mac, 0)]

    def ping(self):
        self.send_data("P,{}\n".format(5000))

    def run(self):
        data = self.client.recv(4096)
        self.prt(data)
        packets = data.split(",")
        self.prt("RECV packets: {}".format(packets))
        if len(packets) < 1:
            return

        # ping packet to keep connection alive
        if packets[0] == "P":
            self.to_ping = (datetime.datetime.now().timestamp()+int(packets[1]))
        elif packets[0] == "M":
            if self.on_recv_handler:
                self.on_recv_handler(self.ap_mac, ",".join(packets[1:]))
        
        if to_ping != 0 and to_ping < datetime.datetime.now().timestamp():
            self.ping()