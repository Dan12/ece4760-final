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

#define DEVICE_ID 1
//
//#define BUFFER_SIZE 1024
//static char read_buffer[BUFFER_SIZE];
//static int buf_ptr = 0;

//void echo_byte_buff() {
//  static int i;
//  static char intbuff[32];
//  for(i = 0; i < buf_ptr; i++) {
//    sprintf(intbuff, "Char: %d", (unsigned int) read_buffer[i]);
//    send_cmd(intbuff, UART_COMP);
//  }
//}
//
//static int most_recent_result = 0;
//
//void echo_buff() {
//  static char result_buff[32];
//  sprintf(result_buff, "Result: %d", most_recent_result);
//  send_cmd(result_buff, UART_COMP);
//  send_cmd(read_buffer, UART_COMP);
//}
//
//#define SEND_CMD_OK_AND_ECHO(cmd) \
//send_cmd(cmd, UART_ESP); \
//buf_ptr = BUFFER_SIZE; \
//most_recent_result = get_data_dma(DEFAULT_TIMEOUT, "OK\r\n", 0, read_buffer, &buf_ptr); \
//echo_buff(); \
//if (most_recent_result != 0) return 1;
//
//static char cmd_buf[256];
//
//int send_data(char* data) {
//  int len = strlen(data);
//  sprintf(cmd_buf, "AT+CIPSEND=1,%d", len);
//  SEND_CMD_OK_AND_ECHO(cmd_buf);
//  
//  while(*data != NULL) {
//    send_byte(*(data++), UART_ESP);
//  }
//  
//  buf_ptr = BUFFER_SIZE;
//  most_recent_result = get_data_dma(DEFAULT_TIMEOUT, "OK\r\n", 0, read_buffer, &buf_ptr);
//  echo_buff();
//  if (most_recent_result != 0) return 1;
//  
//  return 0;
//}

int setup() {
//  send_cmd("Initiating connection", UART_COMP);
//  
//  SEND_CMD_OK_AND_ECHO("AT");
//          
//  SEND_CMD_OK_AND_ECHO("AT");
//          
//  // get info
//  SEND_CMD_OK_AND_ECHO("AT+GMR");
//  
//  // set to Soft AP + Station mode
//  SEND_CMD_OK_AND_ECHO("AT+CWMODE=3");
//  
//  // set name (ssid), password, channel, enc
//  sprintf(cmd_buf, "AT+CWSAP_CUR=\"ESP8266-%d\",\"1234567890\",5,3", DEVICE_ID);
//  SEND_CMD_OK_AND_ECHO(cmd_buf);
//  
//  // set local IP address to a different number
//  sprintf(cmd_buf, "AT+CIPAP_CUR=\"192.168.%d.1\",\"192.168.%d.1\",\"255.255.255.0\"", DEVICE_ID, DEVICE_ID);
//  SEND_CMD_OK_AND_ECHO(cmd_buf);
//  
//  // allow multiple connections (necessary for TCP server)
//  SEND_CMD_OK_AND_ECHO("AT+CIPMUX=1");
//  
//  // create a TCP server on port 80
//  SEND_CMD_OK_AND_ECHO("AT+CIPSERVER=1,80");
//  
//  // get local ip
//  SEND_CMD_OK_AND_ECHO("AT+CIFSR");
  
  return 0;
}

int make_connection(char* ssid) {
//  sprintf(cmd_buf, "AT+CWJAP_CUR=\"%s\",\"1234567890\"", ssid);
//  SEND_CMD_OK_AND_ECHO(cmd_buf);
//  
//  SEND_CMD_OK_AND_ECHO("AT+CIPMUX=1");
//  
//  char* tmp = strtok(ssid, "-");
//  tmp = strtok(NULL, "-");
//  if (tmp != NULL) {
//    sprintf(cmd_buf, "AT+CIPSTART=1,\"TCP\",\"192.168.%s.1\",80", tmp);
//    SEND_CMD_OK_AND_ECHO(cmd_buf);
//    
//    send_data("Hello from the other side\n");
//    send_data("Whats up\n");
//  } else {
//    return 1;
//  }
//  
//  return 0;
}

int main(void) {
  
  init_system();
  
  init_uart();

  INTEnableSystemMultiVectoredInt();
  
  wifi_setup(DEVICE_ID);
  
  if (setup() == 0) {
    send_cmd("Successfully finished setup", UART_COMP); 
  } else {
    send_cmd("Setup failed", UART_COMP); 
    
    // TODO reset
  }
  
  // scan on channel 5
//  send_cmd("AT+CWLAP", UART_ESP);
//  buf_ptr = BUFFER_SIZE;
//  most_recent_result = get_data_dma(DEFAULT_TIMEOUT, "OK\r\n", 0, read_buffer, &buf_ptr);
//  echo_buff();
//  
//  char* tmp = strtok(read_buffer, "+CWLAP");
//  tmp = strtok(NULL, "+CWLAP");
//  tmp = strtok(NULL, "\"");
//  tmp = strtok(NULL, "\"");
//  while(tmp != NULL) {
//    send_cmd(tmp, UART_COMP);
//    if (starts_with(tmp, strlen(tmp), "ESP8266", 7) == 0) {
//      break;
//    }
//    tmp = strtok(NULL, "+CWLAP");
//    tmp = strtok(NULL, "\"");
//    tmp = strtok(NULL, "\"");
//  }
//  
//  if (tmp != NULL) {
//    if (make_connection(tmp) == 0) {
//      send_cmd("Made connection", UART_COMP); 
//    } else {
//      send_cmd("Connection failed", UART_COMP); 
//    }
//  }
  
  while(1) {
    // constant read loop
//    most_recent_result = get_data_dma(DEFAULT_TIMEOUT, "\r\n", 0, read_buffer, &buf_ptr);
//    echo_buff();
  }
}

