#include "wifi.h"
#include "uart.h"
#include "string_util.h"
#include "timer.h"
#include "logger.h"

static void(*recv_handler)(int mac, char* msg);
static void(*disconnection_handler)(int mac);
static void(*connection_handler)(int mac);

#define MAX_CONNECTIONS 5
// Holds the mappings from link id to all connected macs
static int link_id_to_mac[MAX_CONNECTIONS];
// holds the mac of the currently connected ap
static int connected_ap_mac = 0;

#define PING_MS 5000
#define PING_MS_STR "5000"
static int time_to_ping[MAX_CONNECTIONS];

// null terminated data structure to cache visible macs
#define MAX_VISIBLE_MACS 10
static visible_mac visible_macs[MAX_VISIBLE_MACS];

// buffer for reading the data from the serial layer
#define BUFFER_SIZE 1024
static char _read_buffer[BUFFER_SIZE];
static char* read_buffer = _read_buffer;
static int buf_ptr = 0;
static int most_recent_result = 0;

// The AP mac address of the wifi module
static int module_mac = 0;

int get_module_mac() {
  return module_mac;
}

enum INFO_TYPE {
  DATA,
  OPENED,
  CLOSED
};

typedef struct information_packet {
  enum INFO_TYPE t;
  int link_id;
  char* data;
} information_packet;
#define MAX_NUM_INFO_PACKETS 20
information_packet packet_buffer[MAX_NUM_INFO_PACKETS];
int packet_buffer_writer = 0;
int packet_buffer_reader = 0;

#define MAX_DATA_BUFFERS 10
char data_buffers[MAX_DATA_BUFFERS][BUFFER_SIZE];
int next_max_data_buffer = 0;

static char logbuff[256];

void allocate_packet(enum INFO_TYPE t, int link_id, char* data) {
  packet_buffer[packet_buffer_writer].t = t;
  packet_buffer[packet_buffer_writer].link_id = link_id;
  packet_buffer[packet_buffer_writer].data = NULL;
  if (data != NULL) {
    char* new_data_buffer = (char*) data_buffers[next_max_data_buffer];
    next_max_data_buffer = (next_max_data_buffer + 1) % MAX_DATA_BUFFERS;
    strcpy(new_data_buffer, data);

    packet_buffer[packet_buffer_writer].data = new_data_buffer;
  }
  packet_buffer_writer = (packet_buffer_writer + 1) % MAX_NUM_INFO_PACKETS;
}

void process_line(char* line) {
  sprintf(logbuff, "line: %s", line);
  comp_log("WIFI_DBG", logbuff);
  int len = strlen(line);
  if (starts_with(line, len, "+IPD", 4)) {
    sprintf(logbuff, "proc data: %s", line);
    comp_log("WIFI_DBG", logbuff);
    char* tmp = strp(&line, "IPD");
    tmp = strp(&line, ",");
    int link_id = atoi(strp(&line, ","));
    int len = atoi(strp(&line, ":"));
    char* msg = strp(&line, "\n");
    allocate_packet(DATA, link_id, msg);
  } else if (ends_with(line, len, ",CONNECT\r\n", 10)) {
    int link_id = atoi(strp(&line, ","));
    sprintf(logbuff, "proc connect: %d", link_id);
    comp_log("WIFI_DBG", logbuff);
    allocate_packet(OPENED, link_id, NULL);
  } else if (ends_with(line, len, ",CLOSED\r\n", 9)) {
    int link_id = atoi(strp(&line, ","));
    sprintf(logbuff, "proc close: %d", link_id);
    comp_log("WIFI_DBG", logbuff);
    allocate_packet(CLOSED, link_id, NULL);
  }
}

void link_id_disconnected(int link_id) {
  int mac = link_id_to_mac[link_id];
  if (mac != 0) {
    if (mac == connected_ap_mac) {
      connected_ap_mac = 0;
      link_id_to_mac[link_id] = 0;
    }
    
    if (disconnection_handler != NULL)
      disconnection_handler(mac);
  }
}

static char line_buffer[1024];
/**
 * Fills the read buffer and does some line processing
 */
void fill_read_buffer() {
  buf_ptr = BUFFER_SIZE;
  most_recent_result = get_esp_data(DEFAULT_TIMEOUT, read_buffer, &buf_ptr);
  char* tmp = strstr(read_buffer, "\n")+1;
  char* prev_tmp = read_buffer;
  while(tmp != (void*) 1) {
    strncpy(line_buffer, prev_tmp, (int) (tmp - prev_tmp));
    line_buffer[(int) (tmp - prev_tmp)] = '\0';
    if (line_buffer[(int) (tmp - prev_tmp) - 1] == '\r') {
      line_buffer[(int) (tmp - prev_tmp) - 1] = '\0';
    }
    
    process_line(line_buffer);
    
    prev_tmp = tmp;
    tmp = strstr(tmp, "\n")+1;
  }
}

void ping(int link_id) {
  send_data(link_id, "P", PING_MS_STR);
}

void run_pings() {
  int i;
  for (i = 0; i < MAX_CONNECTIONS; i++) {
    if (time_to_ping[i] != 0 && time_to_ping[i] < time_tick_millsec) {
      time_to_ping[i] = 0;
      ping(i);
    }
  }
}

void process_data(int link_id, char* type, char* data) {
    sprintf(logbuff, "proc data: %d %s %s", link_id, type, data);
    comp_log("WIFI_DBG", logbuff);
    if (strcmp(type, "CS") == 0) {
      int mac = parse_mac(data);
      link_id_to_mac[link_id] = mac;

      ping(link_id);

      if (connection_handler != NULL)
        connection_handler(mac);
    } else if (strcmp(type, "P") == 0) {
      time_to_ping[link_id] = time_tick_millsec + PING_MS;
    } else if (strcmp(type, "M") == 0) {
      if (recv_handler != NULL)
        recv_handler(link_id_to_mac[link_id], data);
    }
}

void process_packet_buffer() {
  while(packet_buffer_reader != packet_buffer_writer) {
    information_packet p = packet_buffer[packet_buffer_reader];
    sprintf(logbuff, "proc packet: %d %d %s", p.t, p.link_id, p.data);
    comp_log("WIFI_DBG", logbuff);
    if (p.t == CLOSED) {
      link_id_disconnected(p.link_id);
    } else if (p.t == OPENED) {
      if (!link_id_to_mac[p.link_id]) {
        // set it to not 0 so that it can't be used in get_next_link_id
        link_id_to_mac[p.link_id] = -1; 
      }
    } else if (p.t == DATA) {
      char* next = strchr(p.data, ',');
      if (next) {
        next[0] = '\0';
        next++;
        process_data(p.link_id, p.data, next); 
      }
    }
    
    packet_buffer_reader = (packet_buffer_reader + 1) % MAX_NUM_INFO_PACKETS;
  }
}

void wifi_run() {
  // check if any data comes in
  fill_read_buffer();
  comp_log("WIFI", read_buffer);
  process_packet_buffer();
  run_pings();
}

static char cmd_buf[256];
static char data_buf[1024];

#define SEND_CMD_OK(cmd) \
send_cmd(cmd, UART_ESP); \
fill_read_buffer(); \
comp_log("WIFI", read_buffer); \
if (!most_recent_result) return 0;

int wifi_setup(char id) {
  // reset buffers
  int i;
  for(i = 0; i < MAX_CONNECTIONS; i++) {
    link_id_to_mac[i] = 0;
    time_to_ping[i] = 0;
  }
  
  SEND_CMD_OK("AT");
  
  SEND_CMD_OK("AT");
  
  // setup random IP so that nobody connects to us while we setup
  SEND_CMD_OK("AT+CWSAP_CUR=\"PLZ-DONT-CONNECT\",\"somerandomnoise97501985\",1,3");
  
  // get info
  SEND_CMD_OK("AT+GMR");
  
  // set to Soft AP + Station mode
  SEND_CMD_OK("AT+CWMODE=3");
  
  // set to Soft AP + Station mode
  SEND_CMD_OK("AT+CIPAPMAC_CUR?");
  char* str = read_buffer;
  char* tmp = strp(&str, "CIPAPMAC_CUR:");
  tmp = strp(&str, "\"");
  char* mac = strp(&str, "\"");
  module_mac = parse_mac(mac);
  sprintf(logbuff, "mod mac %d", module_mac);
  comp_log("WIFI_DBG", logbuff);
  
  // set local IP address to a different number
  sprintf(cmd_buf, "AT+CIPAP_CUR=\"192.168.%d.1\",\"192.168.%d.1\",\"255.255.255.0\"", id, id);
  SEND_CMD_OK(cmd_buf);
  
  // allow multiple connections (necessary for TCP server)
  SEND_CMD_OK("AT+CIPMUX=1");
  
  // create a TCP server on port 80
  SEND_CMD_OK("AT+CIPSERVER=1,80");
  
  // set ssid (name), pwd, chnl, enc
  sprintf(cmd_buf, "AT+CWSAP_CUR=\"ESP8266-Mesh-%d\",\"1234567890\",1,3", id);
  SEND_CMD_OK(cmd_buf);
  sprintf(logbuff, "mod mac %d", module_mac);
  comp_log("WIFI_DBG", logbuff);
}

void wifi_register_recv_handler(void(*handler)(int mac, char* msg)) {
  recv_handler = handler;
}

void wifi_register_direct_disconnection_handler(void(*handler)(int mac)) {
  disconnection_handler = handler;
}

void wifi_register_sta_connection_handler(void(*handler)(int sta_mac)) {
  connection_handler = handler;
}

/**
 * 
 * @param link_id
 * @param data
 * @return Return 1 if success, 0 if failure
 */
int send_data(int link_id, char* type, char* data) {
  sprintf(logbuff, "sending data: %s %s", type, data);
  comp_log("WIFI_DBG", logbuff);
  int len = strlen(type) + strlen(data) + 2;
  sprintf(cmd_buf, "AT+CIPSENDBUF=%d,%d", link_id, len);
  SEND_CMD_OK(cmd_buf);
  
  while(*type != NULL) {
    send_byte(*(type++), UART_ESP);
  }
  send_byte(',', UART_ESP);
  while(*data != NULL) {
    send_byte(*(data++), UART_ESP);
  }
  send_byte('\n', UART_ESP);
  
  fill_read_buffer();
  if (!most_recent_result) return 0;
  
  return 1;
}

/**
 * 
 * @param mac
 * @return the link id for this mac, or -1 if no corresponding link_id
 */
int mac_to_link_id(int mac) {
  int i;
  for (i = 0; i < MAX_CONNECTIONS; i++) {
    if (link_id_to_mac[i] == mac) {
      return i;
    }
  }
  return -1;
}

/**
 * 
 * @param dest_mac
 * @param msg
 * @return  1 if success, 0 if failure
 */
int wifi_send_data(int dest_mac, char* msg) {
  int link_id_to_send = mac_to_link_id(dest_mac);
  if (link_id_to_send != -1) {
    if (send_data(link_id_to_send, "M", msg)) {
      // succeeded in sending data
      return 1;
    }
  }
  return 0;
}

char* get_visible_ssid(int mac) {
  int i = 0;
  while(visible_macs[i].mac != 0) {
    if (visible_macs[i].mac == mac) {
      return visible_macs[i].ssid;
    }
  }
  return NULL;
}

/**
 * 
 * @return next unused link id or -1 if no unused link available
 */
int get_next_link_id() {
  int i;
  for (i = 0; i < MAX_CONNECTIONS; i++) {
    sprintf(logbuff, "link id at %d %d", i, link_id_to_mac[i]);
    comp_log("WIFI_DBG", logbuff);
    if (link_id_to_mac[i] == 0) {
      return i;
    }
  }
  return -1;
}

int wifi_connect_to_ap(int ap_mac) {
  if (connected_ap_mac == 0) {
    char* ap_ssid = get_visible_ssid(ap_mac);
    if (ap_ssid != NULL) {
      int ap_id = atoi(ap_ssid+13);
      sprintf(cmd_buf, "AT+CWJAP_CUR=\"%s\",\"1234567890\"", ap_ssid);
      SEND_CMD_OK(cmd_buf);

      // TODO lower timeout time
      int link_id = get_next_link_id();
      sprintf(logbuff, "link id %d", link_id);
      comp_log("WIFI_DBG", logbuff);
      sprintf(cmd_buf, "AT+CIPSTART=%d,\"TCP\",\"192.168.%d.1\",80", link_id, ap_id);
      SEND_CMD_OK(cmd_buf);
      
      // send the AP your information
      sprintf(data_buf, "%d", module_mac);
      if (send_data(link_id, "CS", data_buf)) {
        link_id_to_mac[link_id] = ap_mac;
        connected_ap_mac = ap_mac;
        return 1;
      }
    }
  }
  return 0;
}

/**
 * Disconnect from the ap connected to by connect_to_ap. 
 * If no ap currently connected to, act as NOP
 */
void wifi_disconnect_from_ap() {
  if (connected_ap_mac != 0) {
    int link_id_to_close = link_id_to_mac[connected_ap_mac];
    if (link_id_to_close != -1) {
      send_cmd("AT+CIPCLOSE", UART_ESP); 
      fill_read_buffer();
      if (most_recent_result) {
        int disconnected_mac = connected_ap_mac;
        connected_ap_mac = 0;
        link_id_to_mac[link_id_to_close] = 0;
        disconnection_handler(disconnected_mac);
      }
    }
  }
}

/** 
 * Get the mac of the connected AP
 */
int wifi_get_connected_ap() {
  return connected_ap_mac;
}

static int direct_connections[MAX_CONNECTIONS+1];

int* wifi_get_direct_connections() {
  int i;
  int j = 0;
  for (i = 0; i < MAX_CONNECTIONS; i++) {
    if (link_id_to_mac[i] != 0 && link_id_to_mac[i] != -1) {
      direct_connections[j++] = link_id_to_mac[i];
    }    
  }
  
  for(; j < MAX_CONNECTIONS+1; j++) {
    direct_connections[j] = 0;
  }
  
  return direct_connections;
}

int parse_mac(char* mac) {
  int result = 0;
  char* tmp = strp(&mac, ":");
  while(tmp != NULL) {
    result <<= 8;
    result |= (int) strtol(tmp, NULL, 16);
    tmp = strp(&mac, ":");
  }
  return result;
}

visible_mac* wifi_get_visible_macs() {
  send_cmd("AT+CWLAP", UART_ESP);
  fill_read_buffer();
  comp_log("WIFI", read_buffer);
  
  char* str = read_buffer;
  
  char* tmp = strp(&str, "CWLAP:");
  tmp = strp(&str, "\"");
  int i = 0;
  while(tmp != NULL && most_recent_result) {
    char* ssid = strp(&str, "\"");
    if (starts_with(ssid, strlen(ssid), "ESP8266-Mesh-", 7)) {
      strcpy(visible_macs[i].ssid, ssid);
      
      tmp = strp(&str, ",");
      char* rssi = strp(&str, ",");
      visible_macs[i].strength = -atoi(rssi);
      
      tmp = strp(&str, "\"");
      char* mac = strp(&str, "\"");
      visible_macs[i].mac = parse_mac(mac);
      
      i++;
      if (i >= MAX_VISIBLE_MACS-1) break;
    }
    tmp = strp(&str, "CWLAP:");
    tmp = strp(&str, "\"");
  }
  for (; i < MAX_VISIBLE_MACS; i++) {
    visible_macs[i].mac = 0;
  }
  
  return visible_macs;
}