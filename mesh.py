import serial
import re
import glob
import threading
import time
import datetime
import random

espPat = re.compile("ESP8266-Mesh-\d*\",\d*")
ipRE = re.compile("\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}")
macRE = re.compile('(?:[0-9a-fA-F]:?){12}')

def safe_dec(b):
	return b.decode('utf-8', errors='ignore')

class WifiModule(threading.Thread):
	def __init__(self, port, id, baud="115200"):
		threading.Thread.__init__(self)
		self.port = port
		self.id = id
		ser = serial.Serial(port,baud, timeout=5)
		if ser.isOpen():
			ser.close()

		ser.open()
		ser.isOpen()

		self.ser = ser

		# mapping from ssid to metadata tuple
		self.direct_conns = {}

		self.connected_to_ap = False
		self.to_ping = []

	def linkId_to_mac(self, linkId):
		if linkId in self.direct_conns:
			return self.direct_conns[linkId][2]
		return None

	def mac_to_linkId(self, mac):
		for conn in self.direct_conns:
			if conn[2] == mac:
				return conn[1]
		return None

	def send_data_to_mac(self, mac, data):
		linkId = self.mac_to_linkId(mac)
		# TODO handle no link ID
		self.write_data(linkId, data)

	def get_direct_macs(self):
		ret = []
		for conn in direct_conns:
			ret.append(conn[2])
		return ret

	def iprint(self, str):
		ts = datetime.datetime.now().timestamp()
		now = datetime.datetime.fromtimestamp(ts).strftime('%H:%M:%S')
		print("{}: {}: {}".format(now, self.port, str))

	# Type is 0 if this is a station connected to my AP
	# and a 1 if this is an AP that my station is connected to
	def add_conn(self, type, linkId, mac, IP):
		self.iprint("Adding connection: {} {} {} {}".format(type, linkId, mac, IP))
		self.direct_conns[linkId] = (type, linkId, mac, IP)
		# add a connection to the routing table
		update_table(mac, mac, 1)

	def reserve_conn(self, linkId):
		self.iprint("Reserving connection: {}".format(linkId))
		self.direct_conns[linkId] = None

	def close_conn(self, linkId):
		self.iprint("Closing connection: {}".format(linkId))
		del self.direct_conns[linkId]
		# TODO remove routing table connection and possibly look for a new AP

	def get_next_linkId(self):
		for i in range(5):
			if i in self.direct_conns:
				continue
			else:
				return i
		raise Exception("Out of link IDs")

	def setup(self):
		self.write_cmd("AT")
		self.write_cmd("AT")
		# get info
		self.write_cmd("AT+GMR")
		# setup random IP so that nobody connects to us
		self.write_cmd("AT+CWSAP_CUR=\"{}\",\"{}\",1,0,4,0".format(random.randint(1,10000), random.randint(1,99999999)))
		# switch to station+AP mode
		self.write_cmd("AT+CWMODE=3")
		# get mac
		result = safe_dec(self.write_cmd("AT+CIPSTAMAC_CUR?"))
		self.mac = macRE.findall(result)[0]
		# set ip address to avoid conflicts
		self.ip = "192.168.{}.1".format(self.id)
		self.write_cmd("AT+CIPAP_CUR=\"192.168.{}.1\",\"{}\",\"255.255.255.0\"".format(self.id, self.ip))
		# allow multiple connections (necessary for tcp server)
		self.write_cmd("AT+CIPMUX=1")

		# self.write_cmd("AT+CWSAP_CUR?")
		# passive mode tcp receive (explicit call), not supported in SDK
		# self.write_cmd("AT+CIPRECVMODE=0")
		# get local ip
		# self.write_cmd("AT+CIFSR")

	def broadcast_ap(self):
		# create a tcp server on port 80
		self.write_cmd("AT+CIPSERVER=1,80")
		# set ssid (name), pwd, chnl, enc
		self.write_cmd("AT+CWSAP_CUR=\"ESP8266-Mesh-{}\",\"1234567890\",1,0,4,0".format(self.id))

	def read_line(self):
		line = self.ser.readline()
		self.process_line(safe_dec(line))
		return line

	def write_data(self, linkId, data):
		ser = self.ser

		ser.flushInput()
		self.write_cmd("AT+CIPSEND=" + str(linkId) + "," + str(len(data)) + "\r\n")

		ser.write((data).encode())
		while(1):
			ret = self.read_line()
			self.iprint("Command result: %s" % ret)
			if ("OK" in ret.decode()):
				break
			if ("ERROR" in ret.decode()):
				raise Exception("write data error")

	def write_cmd(self, cmd):
		ser = self.ser

		ser.flushInput()
		ser.write((cmd + "\r\n").encode())

		return self.read_for_ok()

	def read_for_ok(self):
		ser = self.ser

		ret = b''
		while(1):
			line = self.read_line()
			ret += line
			self.iprint("Command result: %s" % line)
			try:
				if ("OK" in line.decode()):
					break
				if ("ERROR" in line.decode()):
					raise Exception("cmd error")
			except UnicodeDecodeError :
				self.iprint("error decoding")
		return ret

	def get_connections(self):
		aps = safe_dec(self.write_cmd("AT+CWLAP"))
		valid = espPat.findall(aps)
		conns = []
		for ap in valid:
			ssid_rsi = ap.split("\",")
			conns.append((ssid_rsi[0], int(ssid_rsi[1])))
		return conns

	def connect_to_ap(self, apId):
		try:
			self.write_cmd("AT+CWJAP_CUR=\"ESP8266-Mesh-{}\",\"1234567890\"".format(apId))
			linkId = self.get_next_linkId()
			self.write_cmd("AT+CIPSTART={},\"TCP\",\"192.168.{}.1\",80".format(linkId, apI))

			# get connection ip
			result = safe_dec(self.write_cmd("AT+CIFSR")).split("\r\n")
			for result_line in result:
				if result_line.startswith("+CIFSR:STAIP"):
					ip = ipRE.findall(result_line)[0]
			if ip and self.mac:
				# send the AP your information
				self.write_data(linkId, "CS,{},{}\n".format(self.mac, ip))

			return True
		except Exception as e:
			self.iprint("There was an exception in the connection proceedure")
			return False

	def process_data(self, linkId, data):
		packets = data.split(",")
		if len(packets) < 1:
			return

		print(packets)
		# connection confirmation from station
		if packets[0] == "CS":
			# self.add_conn(0, linkId, packets[1], packets[2])
			# self.write_cmd("AT+CWLIF")
			# newline packet to allow connection to settle
			# self.write_data(linkId, "\n")
			# self.write_data(linkId, "CA,{},{}\n".format(self.mac, self.ip))
			print(packets)
		# connection confirmation from AP
		elif packets[0] == "CA":
			self.add_conn(1, linkId, packets[1], packets[2])
			# newline packet to allow connection to settle
			self.write_data(linkId, "\n")
			self.ping(linkId)
		# ping packet to keep connection alive
		elif packets[0] == "P":
			self.to_ping.append((linkId,datetime.datetime.now().timestamp()+int(packets[1])))

	def ping(self, linkId):
		self.write_data(linkId, "P,5\n")

	def process_line(self, line):
		if line.startswith("+STA_CONNECTED"):
			pass
		elif line.startswith("+IPD"):
			idx1 = line.index(",")
			idx2 = line.index(",", idx1+1)
			idx3 = line.index(":", idx2+1)
			linkId = int(line[idx1+1:idx2])
			dataLen = int(line[idx2+1:idx3])
			# strip newline
			data = line[idx3+1:idx3+dataLen]
			self.process_data(linkId, data)
			# self.iprint("got data for {} of len {}".format(linkId, dataLen))
		elif line.startswith("+DIST_STA_IP"):
			pass
		elif line.startswith("+LINK_CONN"):
			pass
		elif line.endswith(",CONNECT\r\n"):
			linkId = int(line[:-len(",CONNECT\r\n")])
			self.reserve_conn(linkId)
		elif line.endswith(",CLOSED\r\n"):
			linkId = int(line[:-len(",CLOSED\r\n")])
			self.close_conn(linkId)

	def run(self):
		# self.write_cmd("AT+RST")
		# ser = self.ser
		# while(True):
		# 	line = self.read_line()
		# 	# self.iprint("Read bytes: %s" % line)
		# 	if safe_dec(line).startswith("ready"):
		# 		break

		# time.sleep((self.id-1)*10)
		self.iprint("Running module thread {} on port {}".format(self.id, self.port))
		self.setup()
		# conns = self.get_connections()
		# print(conns)
		# for (ssid, rssi) in conns:
		# 	connId = ssid[len("ESP8266-Mesh-"):]
		# 	if self.connect_to_ap(connId):
		# 		break
		self.broadcast_ap()
		ser = self.ser
		while(True):
			line = self.read_line()
			self.iprint("Read bytes: %s" % line)

			# figure out if I have to ping someone
			# too_early = []
			# while self.to_ping:
			# 	(toPingLink, timeToPing) = self.to_ping.pop()
			# 	if timeToPing < datetime.datetime.now().timestamp():
			# 		self.ping(toPingLink)
			# 	else:
			# 		too_early.append((toPingLink, timeToPing))
			# for elt in too_early:
			# 	self.to_ping.append(elt)

id = 2
for port in glob.glob("/dev/cu.SLAB_USBtoUART7*"):
	module = WifiModule(port,id)
	module.start()
	id+=1
