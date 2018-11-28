#ifndef APP_H
#define	APP_H

//setup the LEDS/any other peripherals
void peripheral_setup();

void peripheral_register_recv_handler(void(*handler)(int mac, char *msg));

void recieved_message(int from_mac, char* msg);



#endif	/* APP_H */