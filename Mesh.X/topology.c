#include <stdio.h>

#include "topology.h"
#include "routing.h"
#include "string_util.h"

static void(*recv_handler)(int mac, char* msg);

void received_message(int from_mac, char* msg) {
  char* type = strp(&msg, ",");
  if (type[0] == 'M') {
    if (recv_handler != NULL) {
      recv_handler(from_mac, msg); 
    }
  }
}

int topology_setup() {
  routing_register_recv_handler(received_message);
}

void topology_register_recv_handler(void(*handler)(int mac, char* msg)) {
  recv_handler = handler;
}

char* send_buf[1024];
void topology_send_msg(int dest_mac, char* msg) {
  sprintf(send_buf, "M,%s", msg);
  routing_send_message(dest_mac, send_buf);
}

int* nodes[11];
int* topology_get_nodes() {
  graph_entry* entries = routing_get_graph();
  int i = 0;
  while(entries[i].mac) {
    nodes[i] = entries[i].mac;
    i++;
  }
  for (; i < 11; i++) {
    nodes[i] = 0;
  }
  return nodes;
}

int mac_in_direct_conns(int mac) {
  int i = 0;
  int* dir_conns = routing_get_direct_connections();
  while(dir_conns[i]) {
    if (dir_conns[i] == mac) {
      return 1;
    }
    i++;
  }
  return 0;
}

int mac_in_graph(int mac) {
  int i = 0;
  int* nodes = topology_get_nodes();
  while(nodes[i]) {
    if (nodes[i] == mac) {
      return 1;
    }
    i++;
  }
  return 0;
}

void topology_run() {
  // if not connected to an ap, find a node not in the graph and connect to it
  if (!wifi_get_connected_ap()) {
    visible_mac* vis_macs = routing_get_visible_macs();
    int i = 0;
    int max_mac = 0;
    int max_stren = -1;
    static char log_buf[256];
    while(vis_macs[i].mac) {
      sprintf(log_buf, "Vis mac: %d %s %d", vis_macs[i].mac, vis_macs[i].ssid, vis_macs[i].strength);
      comp_log("TOPO", log_buf);
      if (!mac_in_direct_conns(vis_macs[i].mac) && !mac_in_graph(vis_macs[i].mac)) {
        if (max_stren == -1 || vis_macs[i].strength > max_stren) {
          max_stren = vis_macs[i].strength;
          max_mac = vis_macs[i].mac;
        } 
      }
    }
    
    if (max_mac) {
      sprintf(log_buf, "Connecting to ap");
      comp_log("TOPO", log_buf);
      wifi_connect_to_ap(max_mac); 
    }
  }
}
