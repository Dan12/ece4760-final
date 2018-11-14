import serial

def write_data(data, linkID=None):
	ser.flushInput()
	if linkID != None:
		ser.write(("AT+CIPSEND=" + str(linkID) + "," + str(len(data)) + "\r\n").encode())
	else:
		ser.write(("AT+CIPSEND=" + str(len(data)) + "\r\n").encode())
	while(1):
		ret = ser.readline()
		print("Command result: %s" % ret)
		if ("OK" in ret.decode()):
			break
		if ("ERROR" in ret.decode()):
			exit()
	ser.write((data).encode())
	while(1):
		ret = ser.readline()
		print("Command result: %s" % ret)
		if ("OK" in ret.decode()):
			break
		if ("ERROR" in ret.decode()):
			exit()

def write_cmd(cmd):
	ser.flushInput()
	ser.write((cmd + "\r\n").encode())
	while(1):
		ret = ser.readline()
		print("Command result: %s" % ret)
		try:
			if ("OK" in ret.decode()):
				break
			if ("ERROR" in ret.decode()):
				exit()
		except UnicodeDecodeError :
			print("error decoding")

def read_loop(resp=False):
	while(1):
		ret = ser.readline()
		print("Command result: %s" % ret)
		if resp:
			try:
				if ("IPD" in ret.decode()):
					write_data("Got response :)\n", linkID=0)
			except UnicodeDecodeError :
				print("error decoding")
			

mod = "blue"

if mod == "blue":
	port = "/dev/cu.SLAB_USBtoUART20"
	speed = "115200"

	ser = serial.Serial(port,speed)
	if ser.isOpen():
		ser.close()

	ser.open()
	ser.isOpen()

	write_cmd("AT")
	write_cmd("AT")
	# get info
	write_cmd("AT+GMR")
	# switch to station+AP mode
	write_cmd("AT+CWMODE=3")
	# set ssid (name), pwd, chnl, enc
	write_cmd("AT+CWSAP_CUR=\"ESP8266-Blue\",\"1234567890\",5,3")
	# list access points
	write_cmd("AT+CWLAP")
	# connect to other device
	write_cmd("AT+CWJAP_CUR=\"ESP8266-Black\",\"1234567890\"")
	# get connection ip
	write_cmd("AT+CIFSR")

	# single connection mode
	write_cmd("AT+CIPMUX=0")

	# connect to tcp server on port 80
	write_cmd("AT+CIPSTART=\"TCP\",\"192.168.5.1\",80")
	# write some data
	write_data("abxy\n")
	write_data("rrff55\n")
	read_loop()
else:
	port = "/dev/cu.SLAB_USBtoUART"
	speed = "115200"

	ser = serial.Serial(port,speed)
	if ser.isOpen():
		ser.close()

	ser.open()
	ser.isOpen()

	write_cmd("AT")
	write_cmd("AT")
	# get info
	write_cmd("AT+GMR")
	# switch to station+AP mode
	write_cmd("AT+CWMODE=3")
	# set name
	write_cmd("AT+CWSAP_CUR=\"ESP8266-Black\",\"1234567890\",5,3")
	# set ip address
	write_cmd("AT+CIPAP_CUR=\"192.168.5.1\",\"192.168.5.1\",\"255.255.255.0\"")
	# allow multiple connections (necessary for tcp server)
	write_cmd("AT+CIPMUX=1")
	# active mode tcp receive
	# write_cmd("AT+CIPRECVMODE=0")
	# create a tcp server on port 80
	write_cmd("AT+CIPSERVER=1,80")
	# get local ip
	write_cmd("AT+CIFSR")
	# listen from connections from remote ip 192.168.4.2
	# write_cmd("AT+CIPSTART=0,\"UDP\",\"192.168.4.2\",4445,4445,2")
	read_loop(resp=True)