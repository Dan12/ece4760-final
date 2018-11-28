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
	// setting RB1 as an output and seetting the output to zero for LED
	mPortBClearBits(BIT_1);
	mPortBClearBits(BIT_1);
    //setting RB2 as an input 
	mPORTBSetPinsDigitalIn(BIT_2);

}


void peripheral_register_recv_handler(void(*handler)(int mac, char *msg)){
	recv_handler = handler;
}

void recieved_message(int from_mac, char* msg) {

	char* type = strp(&msg, ",");
  if (recv_handler != NULL) {
  	recv_handler(from_mac, msg); 
    if (type[0] == 'L') {
    	if(type[1] == '1'){
      mPortBSetBits(BIT_0);
  }else if (type[1] == '2'){
  	  mPortBSetBits(BIT_1);
  }
    }
  }

}
  





