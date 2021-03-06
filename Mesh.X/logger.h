/* 
 * File:   logger.h
 * Author: dantech
 *
 * Created on November 21, 2018, 10:48 PM
 */

#ifndef LOGGER_H
#define	LOGGER_H

#include "uart.h"

// make sure we have enough space for msg + layer
static char log_buff[1124];
static void comp_log(char* layer, char* msg) {
    sprintf(log_buff, "%s: %s", layer, msg);
    send_cmd(log_buff, UART_COMP);
}

#endif	/* LOGGER_H */

