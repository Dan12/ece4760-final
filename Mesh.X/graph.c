#include <stdio.h>
#include <string.h>

#include "graph.h"  
#include "logger.h"

char log_buf[256];
void dump_graph() {
  comp_log("Graph", "Dumping graph");
  int i;
  for (i = 0; i < MAX_NUM_NODES; i++) {
    if (graph[i].mac) {
      sprintf(log_buf, "G: %d %d", graph[i].mac, graph[i].seq_num);
      comp_log("Graph", log_buf);
      int j;
      for (j = 0; j < MAX_NUM_CONNECTIONS; j++) {
        if (graph[i].adj_nodes[j].mac) {
          sprintf(log_buf, "    %d: %d %d", j, graph[i].adj_nodes[j].mac, graph[i].adj_nodes[j].type);
          comp_log("Graph", log_buf); 
        }
      }
    }
  }
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
    sprintf(log_buf, "mac doesn't exist");
    comp_log("Routing", log_buf);
    int i;
    for (i = 0; i < MAX_NUM_NODES; i++) {
      if (graph[i].mac == 0) {
        sprintf(log_buf, "found empty entry %d", i);
        comp_log("Routing", log_buf);
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
      sprintf(log_buf, "found empty entry for edge %d", i);
      comp_log("Routing", log_buf);
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
