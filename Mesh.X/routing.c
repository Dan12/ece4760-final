#include <stdio.h>
#include <string.h>

#include "routing.h"
#include "string_util.h"
#include "logger.h"

static char send_buf[1024];

#define MAX_NUM_NODES 10
graph_entry graph[MAX_NUM_NODES+1];

graph_entry* routing_get_graph() {
  return graph;
}

/**
 * Dump this nodes current view of the network to the comp uart
 */
void dump_graph() {
  comp_log("Routing", "Dumping graph");
  int i;
  for (i = 0; i < MAX_NUM_NODES; i++) {
    if (graph[i].mac) {
      sprintf(send_buf, "G: %d %d", graph[i].mac, graph[i].seq_num);
      comp_log("Routing", send_buf);
      int j;
      for (j = 0; j < MAX_NUM_CONNECTIONS; j++) {
        if (graph[i].adj_nodes[j].mac) {
          sprintf(send_buf, "    %d: %d %d", j, graph[i].adj_nodes[j].mac, graph[i].adj_nodes[j].type);
          comp_log("Routing", send_buf); 
        }
      }
    }
  }
}

/**
 * Get a pointer to the graph entry for the given mac. Returns 
 * an empty graph entry if not found (mac = 0)
 * @param mac
 * @return 
 */
graph_entry* get_graph_entry(int mac) {
  int i;
  for (i = 0; i < MAX_NUM_NODES; i++) {
    if (graph[i].mac == mac) {
      return &graph[i];
    }
  }
  return &graph[MAX_NUM_NODES];
}

/**
 * Get the adj node entry that is connected to the given graph node
 * and has the given mac. Returns an empty adj node entry if not found (mac = 0)
 * @param e
 * @param mac
 * @return 
 */
adj_node_entry* get_adj_node(graph_entry* e, int mac) {
  int j;
  for (j = 0; j < MAX_NUM_CONNECTIONS; j++) {
    if (e->adj_nodes[j].mac == mac) {
      return &e->adj_nodes[j];
    }
  }
  return &empty_adj_node_entry;
}

/**
 * @param mac the mac to search for
 * @return 1 if mac has an entry in the graph, 0 otherwise
 */
int node_exists(int mac) {
  return get_graph_entry(mac)->mac == mac;
}

/**
 * Reset the adj node list for graph entry i
 * @param i
 */
void clear_adj_node_list(int i) {
  int j;
  for (j = 0; j < MAX_NUM_CONNECTIONS; j++) {
    graph[i].adj_nodes[j].mac = 0;
    graph[i].adj_nodes[j].type = 0;
  }
}

/**
 * Create a node entry in the graph with the given mac address if no
 * node with that mac address exists
 * @param mac
 * @return 1 if new node created, 0 if node already exists or no space left
 */
int create_node(int mac) {
  if (!node_exists(mac)) {
    sprintf(send_buf, "mac doesn't exist");
    comp_log("Routing", send_buf);
    int i;
    for (i = 0; i < MAX_NUM_NODES; i++) {
      if (graph[i].mac == 0) {
        sprintf(send_buf, "found empty entry %d", i);
        comp_log("Routing", send_buf);
        graph[i].mac = mac;
        graph[i].seq_num = 0;
        clear_adj_node_list(i);
        return 1;
      }
    }
  }
  return 0;
}

/**
 * Create an edge in the graph from node represented by e to the given mac.
 * @param e one endpoint of the edge
 * @param mac the other endpoint of the edge
 * @param type the type of edge: 1 means that e is the station and mac is the 
 * ap, 0 means e is the ap and mac is the station
 * @return 2 if the edge was updated, 1 if the edge was created, 0 of the edge
 * was not created
 */
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
      sprintf(send_buf, "found empty entry for edge %d", i);
      comp_log("Routing", send_buf);
      e->adj_nodes[i].mac = mac;
      e->adj_nodes[i].type = type;
      return 1;
    }
  }
  return 0;
}

/**
 * Add an edge to the graph if th sequence number is valid.
 * Must go in both directions. Also, update the sequence number
 * @param mac_ap the mac address of the ap
 * @param mac_sta the mac address of the station
 * @param seq_num the sequence number of the station claiming to create the edge
 */
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

/**
 * Convert a mac address to its graph entry index
 * @param mac
 * @return 
 */
int get_mac_graph_index(int mac) {
  int i;
  for (i = 0; i < MAX_NUM_NODES; i++) {
    if (graph[i].mac == mac) {
      return i;
    }
  }
  return MAX_NUM_NODES;
}

/**
 * Remove all disconnected nodes from the graph
 */
void prune_graph() {
  graph_entry* queue[MAX_NUM_NODES];
  int queue_read = 0;
  int queue_write = 0;
  int visited[MAX_NUM_NODES];
  int module_mac = get_module_mac();
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

/**
 * Remove an edge between two macs and prune the graph of disconnected
 * components if the sequence number is valid for the remove edge action
 * @param mac_1
 * @param mac_2
 * @param seq_num
 */
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

/**
 * Send a message to the given neighboring node
 * @param next_hop
 * @param msg
 * @return 
 */
int send_msg(int next_hop, char* msg) {
  return wifi_send_data(next_hop, msg);
}

typedef struct station_connection {
  int sta_mac;
  int sta_seq_num;
  int ap_mac;
} station_connection;
station_connection station_connections[MAX_NUM_NODES+1];

/**
 * sets the station connections list to a list of all edges of type 1
 */
void get_station_connections() {
  int s = 0;
  int i;
  for (i = 0; i < MAX_NUM_NODES; i++) {
    if (graph[i].mac) {
      int j;
      for (j = 0; j < MAX_NUM_CONNECTIONS; j++) {
        if (graph[i].adj_nodes[j].mac) {
          if (graph[i].adj_nodes[j].type == 1) {
            station_connections[s].sta_mac = graph[i].mac;
            station_connections[s].sta_seq_num = graph[i].seq_num;
            station_connections[s++].ap_mac = graph[i].adj_nodes[j].mac;
          }
        }
      }
    }
  }
  
  for(; s < MAX_NUM_NODES+1; s++) {
    station_connections[s].sta_mac = 0;
    station_connections[s].sta_seq_num = 0;
    station_connections[s].ap_mac = 0;
  }
}

/**
 * Send a bootstrap packet to the given mac address
 * @param bootstrap_mac
 */
void send_bootstrap(int bootstrap_mac) {
  char* buf = send_buf;
  sprintf(buf, "B");
  buf+=strlen(buf);
  
  get_station_connections();
  int i = 0;
  while(station_connections[i].sta_mac) {
    station_connection s = station_connections[i];
    sprintf(buf, ",%d,%d,%d", s.sta_mac, s.sta_seq_num, s.ap_mac);
    buf+=strlen(buf);
    i++;
  }
  send_msg(bootstrap_mac, send_buf);
}

/**
 * Send a directed packet with the given information to the next hop
 * @param next_hop
 * @param orig_mac
 * @param orig_seq_num
 * @param dest_mac
 * @param msg
 * @return 
 */
int send_directed(int next_hop, int orig_mac, int orig_seq_num, int dest_mac, char* msg) {
  sprintf(send_buf, "D,%d,%d,%d,%s", orig_mac, orig_seq_num, dest_mac, msg);
  return send_msg(next_hop, send_buf);
}

/**
 * Send a flood packet with the given information to all neighbors
 * @param orig_mac
 * @param orig_seq_num
 * @param f_type
 * @param mac_1
 * @param mac_2
 */
void send_flood(int orig_mac, int orig_seq_num, int f_type, int mac_1, int mac_2) {
  sprintf(send_buf, "F,%d,%d,%d,%d,%d", orig_mac, orig_seq_num, f_type, mac_1, mac_2);
  int i = 0;
  int* dir_conns = wifi_get_direct_connections();
  while(dir_conns[i]) {
    send_msg(dir_conns[i], send_buf); 
    i++;
  }
}

static void(*recv_handler)(int mac, char* msg);
static int seq_num;

/**
 * @param mac
 * @return 1 if the given mac is directly connected to this node, 0 otherwise
 */
int is_in_direct_conns(int mac) {
  int i = 0;
  int* direct_conns = wifi_get_direct_connections();
  while(direct_conns[i]) {
    sprintf(send_buf, "direct connection %d", direct_conns[i]);
    comp_log("Routing", send_buf);
    if (direct_conns[i] == mac) {
      return 1;
    }
    i++;
  }
  return 0;
}

void inc_seq_num() {
  seq_num++;
  int module_mac = get_module_mac();
  graph_entry* m = get_graph_entry(module_mac);
  if (m->mac == module_mac) {
    m->seq_num = seq_num;
  }
}

void mac_disconnected(int mac) {
  sprintf(send_buf, "Disconnecting %d", mac);
  comp_log("Routing", send_buf);
  if (!is_in_direct_conns(mac)) {
    sprintf(send_buf, "Not in dir conns");
    comp_log("Routing", send_buf);
    inc_seq_num();
    int module_mac = get_module_mac();
    dump_graph();
    remove_edge(module_mac, mac, seq_num);
    dump_graph();
    send_flood(module_mac, seq_num, 1, module_mac, mac); 
  }
}

void sta_connected(int mac) {
  inc_seq_num();
  send_bootstrap(mac);
}

/**
 * Checks if the given seq number is valid for the given mac.
 * Also updates seq number to the given seq number if valid
 * @param mac
 * @param seq_num
 * @return 1 if is valid, 0 if false
 */
int is_valid_seq_number(int mac, int seq_num) {
  graph_entry* e = get_graph_entry(mac);
  sprintf(send_buf, "e %d", e->mac);
  comp_log("Routing", send_buf);
  sprintf(send_buf, "es %d", e->seq_num);
  comp_log("Routing", send_buf);
  sprintf(send_buf, "m %d", mac);
  comp_log("Routing", send_buf);
  if (e->mac == mac) {
    if (e->seq_num < seq_num) {
      e->seq_num = seq_num;
      return 1;
    } else {
      return 0;
    }
  }
  return 1;
}

void on_receive_flood(int prev_mac, int orig_mac, int orig_seq_num, int f_type, int mac_1, int mac_2) {
  if (is_valid_seq_number(orig_mac, orig_seq_num)) {
    if (f_type == 0) {
      // create edge
      add_edge(mac_1, mac_2, orig_seq_num);
    } else if (f_type == 1) {
      // remove edge
      remove_edge(mac_1, mac_2, orig_seq_num);
    }
    send_flood(orig_mac, orig_seq_num, f_type, mac_1, mac_2);
  }
}

void on_receive_direct(int orig_mac, int orig_seq_num, int dest_mac, char* msg) {
  if (is_valid_seq_number(orig_mac, orig_seq_num)) {
    if (dest_mac == get_module_mac()) {
      recv_handler(orig_mac, msg);
    } else {
      send_directed(get_next_hop(dest_mac), orig_mac, orig_seq_num, dest_mac, msg);
    }
  }
}

/**
 * Parse a bootstrap packet in data sent from prev_mac.
 * Flood the new information to the network
 * @param prev_mac
 * @param data
 */
void on_receive_bootstrap(int prev_mac, char* data) {
  get_station_connections();
  station_connection my_side[MAX_NUM_NODES];
  int my_side_head = 0;
  int my_side_in_other[MAX_NUM_NODES];
  int i;
  for (i = 0; i < MAX_NUM_NODES; i++) {
    my_side_in_other[i] = 0;
  }
  station_connection other_side[MAX_NUM_NODES];
  int other_side_head = 0;
  
  sprintf(send_buf, "data %s", data);
  comp_log("Routing", send_buf);
  
  while(data) {
    int sta_mac = atoi(strp(&data, ","));
    int sta_seq_num = atoi(strp(&data, ","));
    int ap_mac = atoi(strp(&data, ","));
    
    int found_conn = 0;
    // check if this connection exists on my side
    for (i = 0; i < MAX_NUM_NODES; i++) {
      station_connection s = station_connections[i];
      if (s.ap_mac == ap_mac && s.sta_mac == sta_mac) {
        if (s.sta_seq_num < sta_seq_num) {
          // the connection is fresher on the other side
          other_side[other_side_head].sta_mac = sta_mac;
          other_side[other_side_head].sta_seq_num = sta_seq_num;
          other_side[other_side_head++].ap_mac = ap_mac;
        } else if (s.sta_seq_num > sta_seq_num) {
          // the connection is fresher on my side
          my_side[my_side_head].sta_mac = sta_mac;
          my_side[my_side_head].sta_seq_num = s.sta_seq_num;
          my_side[my_side_head++].ap_mac = ap_mac;
        }
        my_side_in_other[i] = 1;
        found_conn = 1;
        break;
      }
    }
    
    if (!found_conn) {
      // the connection is not on my side
      other_side[other_side_head].sta_mac = sta_mac;
      other_side[other_side_head].sta_seq_num = sta_seq_num;
      other_side[other_side_head++].ap_mac = ap_mac;
    }
  }
  
  for (i = 0; i < MAX_NUM_NODES; i++) {
    sprintf(send_buf, "msio %d %d", i, my_side_in_other[i]);
    comp_log("Routing", send_buf);
    if (!my_side_in_other[i]) {
      station_connection s = station_connections[i];
      sprintf(send_buf, "sta %d %d %d", s.sta_mac, s.sta_seq_num, s.ap_mac);
      comp_log("Routing", send_buf);
      if (s.sta_mac) {
        my_side[my_side_head].sta_mac = s.sta_mac;
        my_side[my_side_head].sta_seq_num = s.sta_seq_num;
        my_side[my_side_head++].ap_mac = s.ap_mac; 
      }
    }
  }
  
  sprintf(send_buf, "msh %d", my_side_head);
  comp_log("Routing", send_buf);

  sprintf(send_buf, "osh %d", other_side_head);
  comp_log("Routing", send_buf);
  
  for(i = 0; i < my_side_head; i++) {
    send_flood(my_side[i].sta_mac, my_side[i].sta_seq_num, 0, my_side[i].ap_mac, my_side[i].sta_mac);
  }
  for(i = 0; i < other_side_head; i++) {
    send_flood(other_side[i].sta_mac, other_side[i].sta_seq_num, 0, other_side[i].ap_mac, other_side[i].sta_mac);
  }
  
  int module_mac = get_module_mac();
  
  sprintf(send_buf, "mm %d", module_mac);
  comp_log("Routing", send_buf);
  inc_seq_num();
  add_edge(prev_mac, module_mac, seq_num);
  send_flood(module_mac, seq_num, 0, prev_mac, module_mac);
}

void recv_message(int from_mac, char* msg) {
  sprintf(send_buf, "From %d: %s", from_mac, msg);
  comp_log("Routing", send_buf);
  char* type = strp(&msg, ",");
  sprintf(send_buf, "Type %s", type);
  comp_log("Routing", send_buf);
  if (type[0] == 'F') {
    int orig_mac = atoi(strp(&msg, ","));
    sprintf(send_buf, "m %d", orig_mac);
    comp_log("Routing", send_buf);
    int orig_seq_num = atoi(strp(&msg, ","));
    sprintf(send_buf, "s %d", orig_seq_num);
    comp_log("Routing", send_buf);
    int f_type = atoi(strp(&msg, ","));
    sprintf(send_buf, "t %d", f_type);
    comp_log("Routing", send_buf);
    int mac_1 = atoi(strp(&msg, ","));
    sprintf(send_buf, "1 %d", mac_1);
    comp_log("Routing", send_buf);
    int mac_2 = atoi(strp(&msg, ","));
    sprintf(send_buf, "2 %d", mac_2);
    comp_log("Routing", send_buf);
    on_receive_flood(from_mac, orig_mac, orig_seq_num, f_type, mac_1, mac_2);
  } else if (type[0] == 'D') {
    int orig_mac = atoi(strp(&msg, ","));
    int orig_seq_num = atoi(strp(&msg, ","));
    int dest_mac = atoi(strp(&msg, ","));
    on_receive_direct(orig_mac, orig_seq_num, dest_mac, msg);
  } else if (type[0] == 'B') {
    on_receive_bootstrap(from_mac, msg);
  }
  dump_graph();
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
  int module_mac = get_module_mac();
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
    return send_directed(next_hop, get_module_mac(), seq_num, dest_mac, msg);
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
