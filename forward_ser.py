import serial

port = "/dev/cu.SLAB_USBtoUART73"
baud = "9600"

ser = serial.Serial(port,baud)
while(True):
  print(ser.readline())