import serial
import re
import glob
import threading
import time

espPat = re.compile("ESP8266-Mesh-\d*")
ipRE = re.compile("\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}")
macRE = re.compile('(?:[0-9a-fA-F]:?){12}')

def safe_dec(b):
	return b.decode('utf-8', errors='ignore')

class WifiModule(threading.Thread):
	def __init__(self, port, id, baud="115200"):
		threading.Thread.__init__(self)
		self.port = port
		self.id = id
		ser = serial.Serial(port,baud)
		if ser.isOpen():
			ser.close()

		ser.open()
		ser.isOpen()

		self.ser = ser

		# mapping from ssid to metadata tuple
		self.direct_conns = {}

		self.connected_to_ap = False

	def iprint(self, str):
		print("{}: {}".format(self.port, str))

	def add_conn(self, linkId, mac, IP):
		self.iprint("Adding connection: {} {} {}".format(linkId, mac, IP))
		self.direct_conns[linkId] = (linkId, mac, IP)

	def reserve_conn(self, linkId):
		self.iprint("Reserving connection: {}".format(linkId))
		self.direct_conns[linkId] = None

	def close_conn(self, linkId):
		self.iprint("Closing connection: {}".format(linkId))
		del self.direct_conns[linkId]

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
		# switch to station+AP mode
		self.write_cmd("AT+CWMODE=3")
		# set ip address to avoid conflicts
		self.write_cmd("AT+CIPAP_CUR=\"192.168.{}.1\",\"192.168.{}.1\",\"255.255.255.0\"".format(self.id, self.id))
		# allow multiple connections (necessary for tcp server)
		self.write_cmd("AT+CIPMUX=1")

		# self.write_cmd("AT+CWSAP_CUR?")
		# passive mode tcp receive (explicit call), not supported in SDK
		# self.write_cmd("AT+CIPRECVMODE=0")
		# get local ip
		# self.write_cmd("AT+CIFSR")

	def broadcast_ap(self):
		# set ssid (name), pwd, chnl, enc
		self.write_cmd("AT+CWSAP_CUR=\"ESP8266-Mesh-{}\",\"1234567890\",1,0,4,0".format(self.id))
		# create a tcp server on port 80
		self.write_cmd("AT+CIPSERVER=1,80")

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

	# def read_loop(self, resp=False):
	# 	ser = self.ser
	#
	# 	while(1):
	# 		ret = self.read_line()
	# 		self.iprint("Command result: %s" % ret)
	# 		if resp:
	# 			try:
	# 				if ("IPD" in ret.decode()):
	# 					write_data("Got response :)\n", linkId=0)
	# 			except UnicodeDecodeError :
	# 				self.iprint("error decoding

	def find_connection(self):
		# list access points
		aps = safe_dec(self.write_cmd("AT+CWLAP"))
		valid = espPat.findall(aps)
		for ap in valid:
			apId = ap[len("ESP8266-Mesh-"):]
			# try:
			self.write_cmd("AT+CWJAP_CUR=\"ESP8266-Mesh-{}\",\"1234567890\"".format(apId))
			linkId = self.get_next_linkId()
			self.write_cmd("AT+CIPSTART={},\"TCP\",\"192.168.{}.1\",80".format(linkId, apId))

			self.connection_ssid = ap
			self.connected_to_ap = True
			self.add_conn(linkId, ap, "192.168.{}.1".format(apId))
			# get connection ip
			result = safe_dec(self.write_cmd("AT+CIFSR")).split("\r\n")
			for result_line in result:
				if result_line.startswith("+CIFSR:STAIP"):
					ip = ipRE.findall(result_line)[0]
				elif result_line.startswith("+CIFSR:STAMAC"):
					mac = macRE.findall(result_line)[0]
			if ip and mac:
				self.write_data(linkId, "C,{},{}\n".format(ip, mac))
			# self.write_cmd("AT+CIPSTATUS")
			break
			# except Exception as e:
			# 	self.iprint("There was an exception")
			# 	continue

		if self.connected_to_ap:
			self.iprint("Connected to AP")
		else:
			self.iprint("Unable to find mesh AP")

	def process_line(self, line):
		if line.startswith("+STA_CONNECTED"):
			pass
		elif line.startswith("+IPD"):
			idx1 = line.index(",")
			idx2 = line.index(",", idx1+1)
			idx3 = line.index(":", idx2+1)
			linkId = int(line[idx1+1:idx2])
			dataLen = int(line[idx2+1:idx3])
			data = line[idx3:idx3+dataLen]
			self.iprint("got data for {} of len {}".format(linkId, dataLen))
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

		time.sleep((self.id-1)*10)
		self.iprint("Running module thread {} on port {}".format(self.id, self.port))
		self.setup()
		self.find_connection()
		self.broadcast_ap()
		ser = self.ser
		while(True):
			line = self.read_line()
			self.iprint("Read bytes: %s" % line)
			# if safe_dec(line).startswith("+STA_CONNECTED:"):
			# 	# get assigned IP
			# 	line = self.read_line()
			# 	self.iprint("Read bytes: %s" % line)
			# 	line = safe_dec(line)
			# 	if line.startswith("+DIST_STA_IP:"):
			# 		# TODO could throw error
			# 		ip = ipRE.findall(line)[0]
			#
			# 		# get linkId
			# 		line = self.read_line()
			# 		self.iprint("Read bytes: %s" % line)
			# 		line = safe_dec(line)
			# 		if line.endswith(",CONNECT\r\n"):
			# 			linkId = int(line[:-len(",CONNECT\r\n")])
			# 			self.add_conn(linkId, "", ip)
			#
			# 			self.write_data(linkId, "Hello from the AP\n")
				# self.write_cmd("AT+CIPSTATUS")
			# check for connection and disconnection messages
			# send periodic sync messages to all clients
			# read tcp buffer
			# get link buffer lengths
			# AT+CIPRECVLEN?
			# get data from links
			# AT+CIPRECVDATA=<link_id>,<len>
			# 	ret: +CIPRECVDATA:<actual_len>,<data>
			# On connection:
			# 	+STA_CONNECTED:"98:01:a7:a2:2b:53"\r\n'
			# Assigned IP:
			#	+DIST_STA_IP:"98:01:a7:a2:2b:53","192.168.1.2"\r\n'
			# Disconnected:
			#	+STA_DISCONNECTED:"98:01:a7:a2:2b:53"\r\n

id = 1
for port in glob.glob("/dev/cu.SLAB_USBtoUART*"):
	module = WifiModule(port,id)
	module.start()
	id+=1
