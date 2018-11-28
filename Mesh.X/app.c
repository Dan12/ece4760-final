#include <stdio.h>

#include "app.h"
#include "topology.h"
#include "routing.h"
#include "string_util.h"
#include "graph.h"
#include "logger.h"


static void(*recv_handler)(int mac, char* msg);


void peripheral_setup(){
	// setting RB0 as an output and setting the output to zero for LED
	mPORTBSetPinsDigitalOut(BIT_0);
	mPortBClearBits(BIT_0);
	// setting RB1 as an input for pushbutton
	mPORTBSetPinsDigitalIn(BIT_1);

}


void peripheral_register_recv_handler(void(*handler)(int mac, char *msg)){
	recv_handler = handler;
}

void recieved_message(int from_mac, char* msg) {

	char* type = strp(&msg, ",");
  if (type[0] == 'L') {
    if (recv_handler != NULL) {
      recv_handler(from_mac, msg); 
      mPortBSetBits(BIT_0);
    }
  }
  





