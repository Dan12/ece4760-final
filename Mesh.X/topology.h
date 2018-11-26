/* 
 * File:   topology.h
 * Author: dantech
 *
 * Created on November 26, 2018, 10:53 AM
 */

#ifndef TOPOLOGY_H
#define	TOPOLOGY_H

/**
 * Setup the topology layer
 * @return 1 if success, 0 otherwise
 */
int topology_setup();

/**
 * Register the handler that will be called whenever a message is passed
 * through this layer
 * @param handler
 */
void topology_register_recv_handler(void(*handler)(int mac, char* msg));

/**
 * Send a message to the destination address in the network
 * @param dest_mac
 * @param msg
 */
void topology_send_msg(int dest_mac, char* msg);

/**
 * Get a list of mac addresses which represent all the nodes in the network
 * that this node is connected to
 * @return 
 */
int* topology_get_nodes();

/**
 * Run the topology algorithms
 */
void topology_run();

#endif	/* TOPOLOGY_H */

