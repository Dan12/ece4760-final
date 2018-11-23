/* 
 * File:   wifi.h
 * Author: dantech
 *
 * Created on November 21, 2018, 10:44 PM
 */

#ifndef WIFI_H
#define	WIFI_H

// The AP mac address of the wifi module
static int module_mac;

/**
 * Setup the wifi module with the given id
 * @param id the id of the module (TODO make this the lsb of mac)
 * @return 1 if the setup succeeded, 0 else
 */
int wifi_setup(char id);

/**
 * Register a handler for when the wifi module receives a message
 * @param handler
 */
void wifi_register_recv_handler(void(*handler)(unsigned int mac, char* msg));

/**
 * Register a handler for when a mac address disconnects from this wifi module
 * Could be AP or STA
 * @param handler
 */
void wifi_register_direct_disconnection_handler(void(*handler)(unsigned int mac));

/**
 * Register a handler for when a station connects to this wifi module's
 * AP
 * @param handler
 */
void wifi_register_sta_connection_handler(void(*handler)(unsigned int sta_mac));

/**
 * Send the message to the dest_mac (must be neightbor)
 * @param dest_mac the neighboring node to send the msg to
 * @param msg the msg to send
 * @return 1 if we succeeded in sending the message, 0 otherwise
 */
int wifi_send_data(int dest_mac, char* msg);

/**
 * Attempts to connect to the ap given by ap_mac
 * @param ap_mac the mac to connect to
 * @return 1 if success, 0 if failure
 */
int wifi_connect_to_ap(int ap_mac);

/**
 * Disconnect from the ap connected to by connect_to_ap. 
 * If no ap currently connected to, act as NOP
 */
void wifi_disconnect_from_ap();

/** 
 * Get the mac of the connected AP
 */
int wifi_get_connected_ap();

/**
 * Gets the list of stations and aps directly connected to this wifi module
 * @return null terminated (0) list of macs
 */
int* wifi_get_direct_connections();


typedef struct visible_mac {
    int mac;
    int strength;
    char ssid[20];
} visible_mac;

/**
 * Get a list of the modules seen by this wifi module
 * @return null terminated (mac) list of visible_mac structs
 */
visible_mac* wifi_get_visible_macs();

#endif	/* WIFI_H */

