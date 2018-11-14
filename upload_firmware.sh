# esptool.py -p /dev/cu.SLAB_USBtoUART write_flash \
# 0x0000 atbin/boot_v1.1.bin \
# 0x01000 atbin/newest/user1.bin \
# 0x7C000 atbin/esp_init_data_default.bin \
# 0x7E000 atbin/blank.bin

esptool.py -p /dev/cu.SLAB_USBtoUART write_flash \
0x0000 atbin1.6/bin/boot_v1.7.bin \
0x01000 atbin1.6/bin/at/512+512/user1.1024.new.2.bin \
# 0xfc000 atbin1.6/bin/esp_init_data_default_v08.bin \
0x7e000 atbin1.6/bin//blank.bin \
0xfe000 atbin1.6/bin//blank.bin