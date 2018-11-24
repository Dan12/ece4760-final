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

#define DEVICE_ID 1

int main(void) {
  
  init_system();
  
  init_uart();

  INTEnableSystemMultiVectoredInt();
  
  comp_log("APP", "starting system");
  
  if (wifi_setup(DEVICE_ID)) {
    comp_log("APP", "successfully finished setup");
  } else {
    comp_log("APP", "failed setup");
    
    // TODO reset
  }
  
  routing_setup();

  while(1) {
    wifi_run();
  }
}

