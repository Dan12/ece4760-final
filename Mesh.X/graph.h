/* 
 * File:   graph.h
 * Author: dantech
 *
 * Created on November 27, 2018, 3:14 PM
 */

#ifndef GRAPH_H
#define	GRAPH_H

typedef struct adj_node_entry {
    int mac;
    int type;
} adj_node_entry;

static adj_node_entry empty_adj_node_entry = {
 0, 0   
};

#define MAX_NUM_CONNECTIONS 5

typedef struct graph_entry {
    int mac;
    int seq_num;
    adj_node_entry adj_nodes[MAX_NUM_CONNECTIONS];
} graph_entry;

#define MAX_NUM_NODES 10

graph_entry graph[MAX_NUM_NODES+1];

/**
 * Dump this nodes current view of the network to the comp uart
 */
void dump_graph();

/**
 * Get a pointer to the graph entry for the given mac. Returns 
 * an empty graph entry if not found (mac = 0)
 * @param mac
 * @return 
 */
graph_entry* get_graph_entry(int mac);

/**
 * Convert a mac address to its graph entry index
 * @param mac
 * @return 
 */
int get_mac_graph_index(int mac);

#endif	/* GRAPH_H */

