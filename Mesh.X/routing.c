#include "routing.h"

#define MAX_NUM_NODES 10
graph_entry graph[MAX_NUM_NODES];

graph_entry* routing_get_graph() {
  return graph;
}

static void(*recv_handler)(unsigned int mac, char* msg);
static int seq_num;

void mac_disconnected(int mac) {
//  TODO
}

void sta_connected(int mac) {
// TODO
}

void recv_message(int mac, char* msg) {
//  TODO
}

int routing_setup() {
  wifi_register_recv_handler(recv_message);
  wifi_register_direct_disconnection_handler(mac_disconnected);
  wifi_register_sta_connection_handler(sta_connected);
  
  seq_num = 1;
  
  int i = 0;
  for (; i < MAX_NUM_NODES; i++) {
    graph[i].mac = 0;
  }
}

void routing_register_recv_handler(void(*handler)(unsigned int mac, char* msg)) {
  recv_handler = handler;
}

int routing_send_message(int dest_mac, char* msg) {
//  TODO
}

int routing_connect_to_ap(int ap_mac) {
  return wifi_connect_to_ap(ap_mac);
}

void routing_disconnect_from_ap() {
  wifi_disconnect_from_ap();
}

int routing_get_connected_ap() {
  return wifi_get_connected_ap();
}

int* routing_get_direct_connections() {
  return wifi_get_direct_connections();
}

visible_mac* routing_get_visible_macs() {
  return wifi_get_visible_macs();
}
