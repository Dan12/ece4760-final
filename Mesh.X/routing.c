#include <stdio.h>

#include "routing.h"

static char send_buf[1024];

#define MAX_NUM_NODES 10
graph_entry graph[MAX_NUM_NODES+1];

graph_entry* routing_get_graph() {
  return graph;
}

graph_entry* get_graph_entry(int mac) {
  int i;
  for (i = 0; i < MAX_NUM_NODES; i++) {
    if (graph[i].mac == mac) {
      return &graph[i];
    }
  }
  return &graph[MAX_NUM_NODES];
}

adj_node_entry* get_adj_node(graph_entry* e, int mac) {
  int j;
  for (j = 0; j < MAX_NUM_CONNECTIONS; j++) {
    if (e->adj_nodes[j].mac == mac) {
      return &e->adj_nodes[j];
    }
  }
  return &empty_adj_node_entry;
}

int node_exists(int mac) {
  return get_graph_entry(mac)->mac == mac;
}


void clear_adj_node_list(int i) {
  int j;
  for (j = 0; j < MAX_NUM_CONNECTIONS; j++) {
    graph[i].adj_nodes[j].mac = 0;
    graph[i].adj_nodes[j].type = 0;
  }
}

int create_node(int mac) {
  if (!node_exists(mac)) {
    int i;
    for (i = 0; i < MAX_NUM_NODES; i++) {
      if (graph[i].mac == 0) {
        graph[i].mac = 0;
        graph[i].seq_num = 0;
        clear_adj_node_list(i);
        return 1;
      }
    }
  }
  return 0;
}

int create_edge(graph_entry* e, int mac, int type) {
  int i;
  for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
    if (e->adj_nodes[i].mac == mac) {
      e->adj_nodes[i].type = type;
      return 2;
    }
  }
  for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
    if (e->adj_nodes[i].mac == 0) {
      e->adj_nodes[i].mac = mac;
      e->adj_nodes[i].type = type;
      return 1;
    }
  }
  return 0;
}

void add_edge(int mac_ap, int mac_sta, int seq_num) {
  create_node(mac_ap);
  create_node(mac_sta);
  graph_entry* sta_e = get_graph_entry(mac_sta);
  if (sta_e->seq_num <= seq_num) {
    graph_entry* ap_e = get_graph_entry(mac_ap);
    create_edge(ap_e, mac_sta, 0);
    create_edge(sta_e, mac_ap, 1);
    // update sequence number
    sta_e->seq_num = seq_num;
  }
}

int get_mac_graph_index(int mac) {
  int i;
  for (i = 0; i < MAX_NUM_NODES; i++) {
    if (graph[i].mac == mac) {
      return i;
    }
  }
  return MAX_NUM_NODES;
}

void prune_graph() {
  graph_entry* queue[MAX_NUM_NODES];
  int queue_read = 0;
  int queue_write = 0;
  int visited[MAX_NUM_NODES];
  graph_entry* node = get_graph_entry(module_mac);
  // make sure node is in graph
  if (node->mac == module_mac) {
    queue[queue_write++] = node;
    visited[get_mac_graph_index(node->mac)] = 1; 
  }
  
  while(queue_read != queue_write) {
    graph_entry* node = queue[queue_read++];
    int i;
    for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
      if (node->adj_nodes[i].mac != 0) {
        int mac = node->adj_nodes[i].mac;
        int g_idx = get_mac_graph_index(mac);
        if (node_exists(mac) && visited[g_idx] == 0) {
          queue[queue_write++] = get_graph_entry(mac);
          visited[g_idx] = 1; 
        }
      }
    }
  }
  
  int i;
  for (i = 0; i < MAX_NUM_NODES; i++) {
    if (visited[i] == 0) {
      graph[i].mac = 0;
      graph[i].seq_num = 0;
      clear_adj_node_list(i);
    }
  }
}

void remove_edge(int mac_1, int mac_2, int seq_num) {
  if(node_exists(mac_1) && node_exists(mac_2)) {
    graph_entry* e_1 = get_graph_entry(mac_1);
    if (e_1->seq_num <= seq_num) {
      adj_node_entry* adj_node_2 = get_adj_node(e_1, mac_2);
      adj_node_entry* adj_node_1 = get_adj_node(get_graph_entry(mac_2), mac_1);
      
      adj_node_1->mac = 0;
      adj_node_2->mac = 0;
      
      e_1->seq_num = seq_num;
    }
  }
  
  prune_graph();
}

void mac_disconnected(int mac);

int send_msg(int next_hop, char* msg) {
  if (!wifi_send_data(next_hop, msg)) {
    mac_disconnected(next_hop);
    return 0;
  }
  return 1;
}

void send_bootstrap(int bootstrap_mac) {
// TODO
//  sprintf(send_buf, "F,%d,%d,%d,%d,%d", orig_mac, orig_seq_num, f_type, mac_1, mac_2);
//  send_msg(bootstrap_mac, send_buf);
}

int send_directed(int next_hop, int orig_mac, int orig_seq_num, int dest_mac, char* msg) {
  sprintf(send_buf, "D,%d,%d,%d,%s", orig_mac, orig_seq_num, dest_mac, msg);
  return send_msg(next_hop, send_buf);
}

void send_flood(int orig_mac, int orig_seq_num, int f_type, int mac_1, int mac_2) {
  sprintf(send_buf, "F,%d,%d,%d,%d,%d", orig_mac, orig_seq_num, f_type, mac_1, mac_2);
  int i;
  int* dir_conns = wifi_get_direct_connections();
  while(dir_conns[i]) {
    send_msg(dir_conns[i], send_buf); 
    i++;
  }
}

static void(*recv_handler)(int mac, char* msg);
static int seq_num;

int is_in_direct_conns(int mac) {
  int i;
  int* direct_conns = wifi_get_direct_connections();
  while(direct_conns[i]) {
    if (direct_conns[i] == mac) {
      return 1;
    }
    i++;
  }
  return 0;
}

void inc_seq_num() {
  seq_num++;
  graph_entry* m = get_graph_entry(module_mac);
  if (m->mac == module_mac) {
    m->seq_num = seq_num;
  }
}

void mac_disconnected(int mac) {
  if (is_in_direct_conns(mac)) {
    inc_seq_num();
    remove_edge(module_mac, mac, seq_num);
    send_flood(module_mac, seq_num, 1, module_mac, mac); 
  }
}

void sta_connected(int mac) {
  inc_seq_num()
  send_bootstrap(mac);
}

void recv_message(int mac, char* msg) {
//  TODO
}

int routing_setup() {
  wifi_register_recv_handler(recv_message);
  wifi_register_direct_disconnection_handler(mac_disconnected);
  wifi_register_sta_connection_handler(sta_connected);
  
  seq_num = 1;
  
  int i;
  for (i = 0; i < MAX_NUM_NODES; i++) {
    graph[i].mac = 0;
    graph[i].seq_num = 0;
    clear_adj_node_list(i);
  }
}

void routing_register_recv_handler(void(*handler)(int mac, char* msg)) {
  recv_handler = handler;
}

/**
 * Return the mac of the next hop to get to dest_mac
 * @param dest_mac
 * @return 
 */
int get_next_hop(int dest_mac) {
  struct queue_entry {
    graph_entry* node;
    int init_hop;
  };
  struct queue_entry queue[MAX_NUM_NODES];
  int queue_read = 0;
  int queue_write = 0;
  int visited[MAX_NUM_NODES];
  graph_entry* init_node = get_graph_entry(module_mac);
  // make sure node is in graph
  if (init_node->mac == module_mac) {
    queue[queue_write].node = init_node;
    queue[queue_write++].init_hop = 0;
    visited[get_mac_graph_index(init_node->mac)] = 1; 
  }
  
  while(queue_read != queue_write) {
    struct queue_entry entry = queue[queue_read++];
    if (entry.node->mac == dest_mac) {
      return entry.init_hop;
    }
    int i;
    for (i = 0; i < MAX_NUM_CONNECTIONS; i++) {
      if (entry.node->adj_nodes[i].mac != 0) {
        int mac = entry.node->adj_nodes[i].mac;
        int g_idx = get_mac_graph_index(mac);
        if (node_exists(mac) && visited[g_idx] == 0) {
          queue[queue_write].node = init_node;
          if (entry.init_hop == 0) {
            queue[queue_write++].init_hop = mac;
          } else {
            queue[queue_write++].init_hop = entry.init_hop;
          }
          visited[g_idx] = 1; 
        }
      }
    }
  }
  
  return 0;
}

int routing_send_message(int dest_mac, char* msg) {
  int next_hop = get_next_hop(dest_mac);
  if (next_hop) {
    inc_seq_num();
    return send_directed(next_hop, module_mac, seq_num, dest_mac, msg);
  }
  return 0;
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