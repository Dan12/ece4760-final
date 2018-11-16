import serial
import re
import glob
import threading

espPat = re.compile("ESP8266-Mesh-\d*")

class WifiModule(threading.Thread):
	def __init__(self, port, id, baud="115200"):
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

	# TODO add mac address
	def add_conn(self, linkId, ssid, IP):
		self.direct_conns[ssid] = (linkID, ssid, IP)

	def setup(self):
		self.write_cmd("AT")
		self.write_cmd("AT")
		# get info
		self.write_cmd("AT+GMR")
		# switch to station+AP mode
		self.write_cmd("AT+CWMODE=3")
		# set ssid (name), pwd, chnl, enc
		self.write_cmd("AT+CWSAP_CUR=\"ESP8266-Mesh-{}\",\"1234567890\",5,3".format(self.id))
		# set ip address to avoid conflicts
		write_cmd("AT+CIPAP_CUR=\"192.168.{}.1\",\"192.168.{}.1\",\"255.255.255.0\"".format(self.id, self.id))
		# allow multiple connections (necessary for tcp server)
		self.write_cmd("AT+CIPMUX=1")
		# passive mode tcp receive (explicit call)
		self.write_cmd("AT+CIPRECVMODE=1")
		# create a tcp server on port 80
		self.write_cmd("AT+CIPSERVER=1,80")
		# get local ip
		self.write_cmd("AT+CIFSR")

	def write_data(self, data, linkID=None):
		ser = self.ser

		ser.flushInput()
		if linkID != None:
			self.write_cmd("AT+CIPSEND=" + str(linkID) + "," + str(len(data)) + "\r\n")
		else:
			self.write_cmd("AT+CIPSEND=" + str(len(data)) + "\r\n")

		ser.write((data).encode())
		while(1):
			ret = ser.readline()
			print("Command result: %s" % ret)
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
		ret = b''
		while(1):
			line = ser.readline()
			ret += line
			print("Command result: %s" % line)
			try:
				if ("OK" in line.decode()):
					break
				if ("ERROR" in line.decode()):
					raise Exception("cmd error")
			except UnicodeDecodeError :
				print("error decoding")
		return ret

	# def read_loop(self, resp=False):
	# 	ser = self.ser
	#
	# 	while(1):
	# 		ret = ser.readline()
	# 		print("Command result: %s" % ret)
	# 		if resp:
	# 			try:
	# 				if ("IPD" in ret.decode()):
	# 					write_data("Got response :)\n", linkID=0)
	# 			except UnicodeDecodeError :
	# 				print("error decoding")

	def find_connection(self):
		# list access points
		aps = write_cmd("AT+CWLAP").decode('utf-8', errors='ignore')
		valid = espPat.findall(aps)
		for ap in valid:
			apId = ap[len("ESP8266-Mesh-"):]
			try:
				self.write_cmd("AT+CWJAP_CUR=\"ESP8266-Mesh-{}\",\"1234567890\"".format(apId))
				# TODO fix link id
				linkId = 1
				self.write_cmd("AT+CIPSTART={},\"TCP\",\"192.168.{}.1\",80".format(linkId, apId))
			except Exception:
				continue
			self.connection_ssid = ap
			self.connected_to_ap = True
			# TODO fix linkid and ip
			ip = ""
			add_conn(0, ap, ip)
			# get connection ip
			write_cmd("AT+CIFSR")

	def run(self):
		print("Running module thread")
		module.setup()
		module.find_connection()
		# TODO maybe can get remote ip from here
		self.write_cmd("AP+CIPSTATUS")
		while(True):
			line = ser.readline()
			print("Read bytes: %s" % line)
			# check for connection and disconnection messages
			# send periodic sync messages to all clients
			# read tcp buffer
			# get link buffer lengths
			# AT+CIPRECVLEN?
			# get data from links
			# AT+CIPRECVDATA=<link_id>,<len>
			# 	ret: +CIPRECVDATA:<actual_len>,<data>

id = 1
for port in glob.glob("/dev/cu.SLAB_USBtoUART*"):
	module = WifiModule(port,id)
	module.start()
	id+=1
