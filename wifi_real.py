import serial
import re
import time
import datetime
import random
from wifi_api import WifiAPI
from wifi_serial import WifiSerial
from recordclass import recordclass

espPat = re.compile("ESP8266-Mesh-\d*\",-\d*,\"(?:[0-9a-fA-F]:?){12}")
ipRE = re.compile("\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}")
macRE = re.compile('(?:[0-9a-fA-F]:?){12}')

def safe_dec(b):
	return b.decode('utf-8', errors='ignore')

# disconnect time is 180 seconds
PING_TIME = 30

class WifiReal(WifiAPI):

    def __init__(self, port, id, baud="115200"):
        super(WifiReal, self).__init__(str(0))

        self.port = port
        self.id = id
        self.ser = serial.Serial(port,baud, timeout=5)
        if self.ser.isOpen():
            self.ser.close()

        self.ser.open()
        self.ser.isOpen()

        self.serial = WifiSerial(self.ser, self.prt)

        # mapping from mac to linkId
        self.direct_conns = {}
        # mapping from linkId to mac
        self.link_id_table = {}

        # (linkId, mac)
        self.connected_ap = None

        self.to_ping = []

        # map from mac to ssid
        self.visible_modules = {}
        # list of (mac, -rssi)
        self.visible_macs = []

        self.prt("Setting up {} on port {}".format(self.id, self.port))
        self.setup()

    def prt(self, msg):
        ts = datetime.datetime.now().timestamp()
        now = datetime.datetime.fromtimestamp(ts).strftime('%H:%M:%S')
        print("{}: Wifi {},{}: {}".format(now, self.id, self.mac[:2], msg))

    def setup(self):
        self.serial.write_cmd("AT+RST")
        while(True):
            line = self.serial.read_line()
            # self.iprint("Read bytes: %s" % line)
            if safe_dec(line).startswith("ready"):
                break

        self.serial.write_cmd("AT")
        self.serial.write_cmd("AT")
        # get info
        self.serial.write_cmd("AT+GMR")
        # setup random IP so that nobody connects to us while we setup
        self.serial.write_cmd("AT+CWSAP_CUR=\"{}\",\"{}\",1,0,4,0".format(random.randint(1,10000), random.randint(1,99999999)))
        # switch to station+AP mode
        self.serial.write_cmd("AT+CWMODE=3")
        # get mac
        result = safe_dec(self.serial.write_cmd("AT+CIPAPMAC_CUR?"))
        self.mac = macRE.findall(result)[0]
        # mac_lsb = int(self.mac[-2:], 16)
        self.prt(self.mac)
        # set ip address to avoid conflicts
        self.ip = "192.168.{}.1".format(self.id)
        self.serial.write_cmd("AT+CIPAP_CUR=\"192.168.{}.1\",\"{}\",\"255.255.255.0\"".format(self.id, self.ip))
        # allow multiple connections (necessary for tcp server)
        self.serial.write_cmd("AT+CIPMUX=1")
        # create a tcp server on port 80
        self.serial.write_cmd("AT+CIPSERVER=1,80")
        # set ssid (name), pwd, chnl, enc
        self.serial.write_cmd("AT+CWSAP_CUR=\"ESP8266-Mesh-{}\",\"1234567890\",1,3,4,0".format(self.id))

    def handle_incoming_data(self, from_mac, data):
        # self.prt("from: {}".format(from_mac))
        # self.prt("dir conns: {}".format(self.get_direct_connections()))
        if from_mac in self.get_direct_connections():
            self.prt("from {} got: {}".format(from_mac, data))
            if self.on_recv_handler:
                self.on_recv_handler(from_mac, data)

    def send_data(self, dest_mac, data):
        # TODO catch failed send
        if dest_mac in self.direct_conns:
            # Translate to linkId
            self.serial.write_data(self.direct_conns[dest_mac], "M,{}\n".format(data))
            return True
        elif self.connected_ap and dest_mac == self.connected_ap[1]:
            # Translate to linkId
            self.serial.write_data(self.connected_ap[0], "M,{}\n".format(data))
            return True
        return False

    def listen_for_lines(self):
        line = self.serial.read_line()
        # self.prt("Listened for bytes: %s" % line)
        next_line = self.serial.pop_proc_queue()
        if next_line:
            self.process_line(safe_dec(next_line))

    def run(self):
        next_line = self.serial.pop_proc_queue()
        while(next_line):
            # self.prt(next_line)
            self.process_line(safe_dec(next_line))
            next_line = self.serial.pop_proc_queue()

        # TODO better ping
        # figure out if I have to ping someone
        too_early = []
        while self.to_ping:
            (toPingLink, timeToPing) = self.to_ping.pop()
            if timeToPing < datetime.datetime.now().timestamp():
                self.ping(toPingLink)
            else:
                too_early.append((toPingLink, timeToPing))
        for elt in too_early:
            self.to_ping.append(elt)

    def process_data(self, linkId, data):
        packets = data.split(",")
        self.prt("RECV packets: {}".format(packets))
        if len(packets) < 1:
            return

        # connection confirmation from station
        if packets[0] == "CS":
            # officially make the connection
            self.link_id_table[linkId] = packets[1]
            self.direct_conns[packets[1]] = linkId
            # newline packet to allow connection to settle
            # send ping packet
            self.ping(linkId)
            # 
            self.station_connection(packets[1])
        # ping packet to keep connection alive
        elif packets[0] == "P":
            self.to_ping.append((linkId,datetime.datetime.now().timestamp()+int(packets[1])))
        elif packets[0] == "M":
            self.handle_incoming_data(self.link_id_table[linkId], ",".join(packets[1:]))

    def ping(self, linkId):
        self.serial.write_data(linkId, "P,{}\n".format(PING_TIME))

    def process_line(self, line):
        if line.startswith("+IPD"):
            idx1 = line.index(",")
            idx2 = line.index(",", idx1+1)
            idx3 = line.index(":", idx2+1)
            linkId = int(line[idx1+1:idx2])
            dataLen = int(line[idx2+1:idx3])
            # strip newline
            data = line[idx3+1:idx3+dataLen]
            # self.prt("got data for {} of len {}".format(linkId, dataLen))
            self.process_data(linkId, data)
        elif line.endswith(",CONNECT\r\n"):
            linkId = int(line[:-len(",CONNECT\r\n")])
            self.reserve_conn(linkId)
        elif line.endswith(",CLOSED\r\n"):
            linkId = int(line[:-len(",CLOSED\r\n")])
            self.close_conn(linkId)

    def reserve_conn(self, linkId):
        self.prt("Reserving connection: {}".format(linkId))
        if linkId not in self.link_id_table:
            self.link_id_table[linkId] = None

    def close_conn(self, linkId):
        self.prt("Closing connection: {}".format(linkId))
        self.prt("{}, {}, {}".format(linkId, self.link_id_table, self.direct_conns))
        if linkId in self.link_id_table:
            mac = self.link_id_table[linkId]
            self.mac_disconnected(mac)

    def get_next_linkId(self):
        for i in range(5):
            if i in self.link_id_table:
                continue
            else:
                return i
        raise Exception("Out of link IDs")

    def connect_to_ap(self, ap_mac):
        if not self.connected_ap:
            try:
                ap_ssid = self.visible_modules[ap_mac]
                ap_id = int(ap_ssid[len("ESP8266-Mesh-"):])
                self.serial.write_cmd("AT+CWJAP_CUR=\"ESP8266-Mesh-{}\",\"1234567890\"".format(ap_id))
                
                linkId = self.get_next_linkId()
                self.serial.write_cmd("AT+CIPSTART={},\"TCP\",\"192.168.{}.1\",80".format(linkId, ap_id))

                # send the AP your information
                self.serial.write_data(linkId, "CS,{}\n".format(self.mac))

                # assign ap
                self.connected_ap = (linkId, ap_mac)
                self.link_id_table[linkId] = ap_mac
                self.connected_ap = (linkId, ap_mac)

                return True
            except Exception as e:
                self.prt("There was an exception in the connection proceedure")

                return False

    # called when a station connects to this AP
    def station_connection(self, sta_mac):
        # if we already connected to this mac's AP
        if self.connected_ap and self.connected_ap[1] == sta_mac:
            self.prt("duplicate conn: {} {}".format(self.mac, sta_mac))
            if self.mac < sta_mac:
                # we disconnect and let the other mac's station connection through
                # self.disconnect_from_ap()
                self.connected_ap = None
            else:
                # we discard the other mac's station connection as we will become the AP
                return
        
        # invoke handler
        if self.on_ap_connection_handler:
            self.prt("Invoking ap sta conn handler")
            self.on_ap_connection_handler(sta_mac)

    def get_direct_connections(self):
        conns = list(self.direct_conns.keys())
        if self.connected_ap and self.connected_ap[1] not in conns:
            conns.append(self.connected_ap[1])
        return conns

    def disconnect_from_ap(self):
        if self.connected_ap:
            ap_to_disconnect = self.connected_ap
            self.connected_ap = None
            self.serial.write_cmd("AT+CIPCLOSE={}".format(ap_to_disconnect[0]))
            self.on_direct_disconnection_handler(ap_to_disconnect[1])

    def get_connected_ap(self):
        if self.connected_ap:
            return self.connected_ap[1]
        else:
            return None

    def mac_disconnected(self, mac):
        if self.connected_ap and mac == self.connected_ap[1]:
            self.connected_ap = None
        else:
            linkId = self.direct_conns[mac]
            del self.direct_conns[mac]
            del self.link_id_table[linkId]
        if self.on_direct_disconnection_handler:
            self.on_direct_disconnection_handler(mac)

    def get_visible_macs(self):
        aps = safe_dec(self.serial.write_cmd("AT+CWLAP"))
        valid = espPat.findall(aps)
        self.visible_modules = {}
        self.visible_macs = []
        for ap in valid:
            parts = ap.split(",")
            ssid = parts[0][:-1]
            rssi = int(parts[1])
            mac = parts[2][1:]
            self.prt("{} {} {}".format(ssid, rssi, mac))

            self.visible_modules[mac] = ssid
            self.visible_macs.append((mac, -rssi))

        return self.visible_macs