import glob
from wifi_real import WifiReal

id = 1
for port in glob.glob("/dev/cu.SLAB_USBtoUART*"):
    module = WifiReal(port,id)
    print(module.get_visible_macs())
    while(True):
        module.run()
    id+=1