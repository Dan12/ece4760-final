#include <stdio.h>

#include "topology.h"
#include "routing.h"
#include "string_util.h"
#include "graph.h"
#include "logger.h"

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

char send_buf[1024];
void topology_send_msg(int dest_mac, char* msg) {
  sprintf(send_buf, "M,%s", msg);
  routing_send_message(dest_mac, send_buf);
}

int nodes[MAX_NUM_NODES+1];
int* topology_get_nodes() {
  int i = 0;
  while(graph[i].mac) {
    nodes[i] = graph[i].mac;
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

int visited[MAX_NUM_NODES];
int visited_idx = 0;
int is_in_visited(int mac) {
  int i;
  for (i = 0; i < MAX_NUM_NODES; i++) {
    if (visited[i] == mac) {
      return 1;
    }
  }
  return 0;
}

/**
 * @param mac
 * @return 0 if module_mac is not in a cycle or the minimum mac of the cycle
 * that module_mac is in
 */
int is_in_cycle(int mac) {
  if (!is_in_visited(mac)) {
    visited[visited_idx++] = mac;
    graph_entry* entry = get_graph_entry(mac);
    if (entry->mac == mac) {
      int i;
      // iterate over all the connections that are of type 1 (sta to ap)
      for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
        if (entry->adj_nodes[i].mac && entry->adj_nodes[i].type == 1) {
          // result is 
          int result = is_in_cycle(entry->adj_nodes[i].mac);
          if (result) {
            if (result < mac) {
              return result;
            }
            return mac;
          }
        }
      }
    }
  } else if (mac == get_module_mac()) {
    return get_module_mac();
  }
  return 0;
}

int cycle_should_disconnect() {
  visited_idx = 0;
  int i;
  for (i = 0; i < MAX_NUM_NODES; i++) {
    visited[i] = 0;
  }
  
  int cycle_min = is_in_cycle(get_module_mac());
  if (cycle_min && cycle_min == get_module_mac()) {
    return 1;
  }
  return 0;
}

int get_node_in_degree(int mac) {
  graph_entry* entry = get_graph_entry(mac);
  int i;
  int in_degree = 0;
  for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
    if (entry->adj_nodes[i].mac) {
      if (entry->adj_nodes[i].type == 0) {
        in_degree++;
      }
    }
  }
  return in_degree;
}

int has_conn_type_1(graph_entry* entry) {
  int i;
  for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
    if (entry->adj_nodes[i].mac) {
      if (entry->adj_nodes[i].type == 1) {
        return 1;
      }
    }
  }
  return 0;
}

int find_reversable_mac() {
  graph_entry* queue[MAX_NUM_NODES];
  int j;
  for (j = 0; j < MAX_NUM_NODES; j++) {
    visited[j] = 0;
  }
  
  int queue_read = 0;
  int queue_write = 0;
  int module_mac = get_module_mac();
  graph_entry* init_node = get_graph_entry(module_mac);
  // make sure node is in graph
  if (init_node->mac == module_mac) {
    queue[queue_write++] = init_node;
    visited[get_mac_graph_index(init_node->mac)] = 1; 
  }
  
  while(queue_read != queue_write) {
    graph_entry* entry = queue[queue_read++];
    if (!has_conn_type_1(entry)) {
      return entry->mac;
    }
    int i;
    for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
      if (entry->adj_nodes[i].mac != 0) {
        int mac = entry->adj_nodes[i].mac;
        int g_idx = get_mac_graph_index(mac);
        if (node_exists(mac) && visited[g_idx] == 0) {
          queue[queue_write++] = get_graph_entry(mac);
          visited[g_idx] = 1;
        }
      }
    }
  }
  return 0;
}

int mac_path[MAX_NUM_NODES];
int find_path(graph_entry* entry, int end_mac, int i) {
  mac_path[i] = entry->mac;
  if (entry->mac == end_mac) {
    return 1;
  }
  int j;
  for (j = 0; j < MAX_NUM_CONNECTIONS; j++) {
    int mac = entry->adj_nodes[j].mac;
    if (mac) {
      if (find_path(get_graph_entry(mac), end_mac, i+1)) {
        return 1;
      }
    }
  }
  return 0;
}

int find_reverse_edge() {
  
  int i;
  
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
      i++;
    }
    
    if (max_mac) {
      sprintf(log_buf, "Connecting to ap");
      comp_log("TOPO", log_buf);
      wifi_connect_to_ap(max_mac); 
    }
    return;
  }
  
//  Run no cycle algorithm
//  if (cycle_should_disconnect()) {
//    routing_disconnect_from_ap();
//    return;
//  }
}
