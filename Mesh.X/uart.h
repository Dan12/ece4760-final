/* 
 * File:   uart.h
 * Author: dantech
 *
 * Created on November 15, 2018, 5:18 PM
 */

#define _SUPPRESS_PLIB_WARNING 
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING
#include <plib.h>

#ifndef UART_H
#define	UART_H

#define UART_ESP UART1
#define UART_COMP UART2

static UART_MODULE dma_uart_chnl = UART_ESP;

// 5 seconds
#define DEFAULT_TIMEOUT 5000

int get_data(int timeout, UART_MODULE m, char* term, int echo, char* ret_buf, int* len);

int get_data_dma(int timeout, char* term, int echo, char* ret_buf, int* len);

void send_byte(char b, UART_MODULE m);

void send_cmd(char* cmd, UART_MODULE m);

void init_uart();

#endif	/* UART_H */

