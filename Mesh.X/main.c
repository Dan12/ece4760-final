/* 
 * File:   main.c
 * Author: dantech
 * 
 * Pin config:
 * RA1 -> USB TX (Green wire)
 * RB10 -> USB RX (White wire)
 * 
 * RA2 -> ESP TX (next to GND)
 * RB7 -> ESP RX (next to VCC)
 *
 * Created on November 1, 2018, 2:14 PM
 */

#define _SUPPRESS_PLIB_WARNING 
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING
#include <plib.h>
// serial stuff
#include <stdio.h>

#include "config.h"
#include "system.h"
#include "uart.h"
#include "string_util.h"

#include "wifi.h"
#include "logger.h"
#include "routing.h"
#include "topology.h"

volatile int pinMode = 0;
volatile int pinHold = 0;

void __ISR(_TIMER_1_VECTOR, IPL2AUTO) Timer1Handler(void) {
    // clear the interrupt flag
    mT1ClearIntFlag();
    
    if (pinMode || pinHold) {
      mPORTASetBits(BIT_0);
    } else {
      mPORTAClearBits(BIT_0);
    }
    
    pinMode = !pinMode;
}

int main(void) {
  
  init_system();
  
  init_uart();
  
  mPORTASetPinsDigitalOut(BIT_0);    //Set port as output
  
  // ===Set up timer1 ======================
  // timer 5: on,  interrupts, internal clock, 
  // set up to count millsec
  OpenTimer1(T1_ON | T1_SOURCE_INT | T1_PS_1_256, 0xffff);
  // set up the timer interrupt with a priority of 2
  ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_2);
  mT1ClearIntFlag(); // and clear the interrupt flag

  INTEnableSystemMultiVectoredInt();
  
  mPORTASetBits(BIT_0);
  
  comp_log("APP", "starting system");
  
  if (wifi_setup()) {
    comp_log("APP", "successfully finished setup");
  } else {
    comp_log("APP", "failed setup");
    
    WritePeriod1(0xffff/4);
    // Error blinking
    while(1);
  }
  
  routing_setup();
  
  topology_setup();
  
  while(1) {
    wifi_run();
    topology_run();
    if (wifi_get_connected_ap()) {
      pinHold = 1;
    } else {
      pinHold = 0;
    }
  }
}

