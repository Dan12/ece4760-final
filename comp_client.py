import socket
import uuid
import re

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

mesh_id = 1

client.connect(("192.168.{}.1".format(mesh_id), 80))

my_mac = ':'.join(re.findall('..', '%012x' % uuid.getnode()))
# send station connection confirmation
client.send("CS#{}".format(my_mac).encode())

while(True):
    data = client.recv(4096)
    print(data)
