/* 
 * File:   logger.h
 * Author: dantech
 *
 * Created on November 21, 2018, 10:48 PM
 */

#ifndef LOGGER_H
#define	LOGGER_H

#include "uart.h"

static char log_buff[1024];
static void log(char* layer, char* msg) {
    sprintf(log_buff, "%s: %s", layer, msg);
    send_cmd(log_buff, UART_COMP);
}

#endif	/* LOGGER_H */

