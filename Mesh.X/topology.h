/* 
 * File:   topology.h
 * Author: dantech
 *
 * Created on November 26, 2018, 10:53 AM
 */

#ifndef TOPOLOGY_H
#define	TOPOLOGY_H

void topology_register_recv_handler(void(*handler)(int mac, char* msg));

void topology_send_msg(int dest_mac, char* msg);

int* topology_get_nodes();

void topology_run();

#endif	/* TOPOLOGY_H */

