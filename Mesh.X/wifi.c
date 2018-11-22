#include "wifi.h"
#include "uart.h"
#include "string_util.h"

static void(*recv_handler)(unsigned int mac, char* msg);
static void(*disconnection_handler)(unsigned int mac);
static void(*connection_handler)(unsigned int mac);

#define MAX_CONNECTIONS 5
static int link_id_to_mac[MAX_CONNECTIONS];
static int connected_ap_mac = 0;

static int time_to_ping[MAX_CONNECTIONS];

#define MAX_VISIBLE_MACS 10
static visible_mac visible_macs[MAX_VISIBLE_MACS];

#define BUFFER_SIZE 1024
static char read_buffer[BUFFER_SIZE];
static int buf_ptr = 0;
static int most_recent_result = 0;

int wifi_setup(char id) {
  // Call setup code
  static int i;
  for(i = 0; i < MAX_CONNECTIONS; i++) {
    link_id_to_mac[i] = 0;
    time_to_ping[i] = 0;
  }
}

void wifi_register_recv_handler(void(*handler)(unsigned int mac, char* msg)) {
  recv_handler = handler;
}

void wifi_register_direct_disconnection_handler(void(*handler)(unsigned int mac)) {
  disconnection_handler = handler;
}

void wifi_register_sta_connection_handler(void(*handler)(unsigned int sta_mac)) {
  connection_handler = handler;
}

int wifi_send_data(int dest_mac, char* msg) {
//  TODO
}

int wifi_connect_to_ap(int ap_mac);

/**
 * Disconnect from the ap connected to by connect_to_ap. 
 * If no ap currently connected to, act as NOP
 */
void wifi_disconnect_from_ap();

/** 
 * Get the mac of the connected AP
 */
int wifi_get_connected_ap();

/**
 * Gets the list of stations and aps directly connected to this wifi module
 * @return list of macs
 */
int* wifi_get_direct_connections();

int parse_mac(char* mac) {
  int result = 0;
  char* tmp = strtok(mac, ":");
  while(tmp != NULL) {
    result <<= 8;
    result |= (int) strtol(tmp, NULL, 16);
    tmp = strtok(NULL, ":");
  }
  return result;
}

visible_mac* wifi_get_visible_macs() {
  send_cmd("AT+CWLAP", UART_ESP);
  buf_ptr = BUFFER_SIZE;
  most_recent_result = get_data_dma(DEFAULT_TIMEOUT, "OK\r\n", 0, read_buffer, &buf_ptr);
  
  char* tmp = strtok(read_buffer, "CWLAP");
  static int i = 0;
  while(tmp != NULL) {
    send_cmd(tmp, UART_COMP);
    tmp = strtok(NULL, ",");
    char* ssid = strtok(NULL, "\"");
    if (starts_with(ssid, strlen(ssid), "ESP8266", 7) == 0) {
      strcpy(visible_macs[i].ssid, ssid);
      char* rssi = strtok(NULL, ",");
      visible_macs[i].strength = -atoi(rssi);
      char* mac = strtok(NULL, "\"");
      visible_macs[i].mac = parse_mac(mac);
      
      i++;
      if (i >= MAX_VISIBLE_MACS) break;
    }
    tmp = strtok(NULL, "CWLAP");
  }
  
  return visible_macs;
}