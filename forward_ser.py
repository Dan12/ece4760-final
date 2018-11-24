import serial

port = "/dev/cu.SLAB_USBtoUART"
baud = "9600"

ser = serial.Serial(port,baud)
while(True):
  print(ser.readline())