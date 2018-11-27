/* 
 * File:   routing.h
 * Author: dantech
 *
 * Created on November 24, 2018, 12:31 PM
 */

#ifndef ROUTING_H
#define	ROUTING_H

#include "wifi.h"

int routing_setup();

void routing_register_recv_handler(void(*handler)(int mac, char* msg));

/**
 * Send a message to the destination mac
 * @param dest_mac the mac to send the message to
 * @param msg the message to send
 * @return 1 if the message succeeded, 0 if there was a failure, including
 * if no path to the dest mac can be found
 */
int routing_send_message(int dest_mac, char* msg);

/**
 * Attempts to connect to the ap given by ap_mac
 * @param ap_mac the mac to connect to
 * @return 1 if success, 0 if failure
 */
int routing_connect_to_ap(int ap_mac);

/**
 * Disconnect from the ap connected to by connect_to_ap. 
 * If no ap currently connected to, act as NOP
 */
void routing_disconnect_from_ap();

/** 
 * Get the mac of the connected AP
 */
int routing_get_connected_ap();

/**
 * Gets the list of stations and aps directly connected to this wifi module
 * @return null terminated (0) list of macs
 */
int* routing_get_direct_connections();

/**
 * Get a list of the modules seen by this wifi module
 * @return null terminated (mac) list of visible_mac structs
 */
visible_mac* routing_get_visible_macs();

#endif	/* ROUTING_H */

