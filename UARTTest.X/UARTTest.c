/* 
 * File:   UARTTest.c
 * Author: dantech
 *
 * Created on November 1, 2018, 2:14 PM
 */

#define _SUPPRESS_PLIB_WARNING 
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING
#include <plib.h>
// serial stuff
#include <stdio.h>

//=============================================================
// 60 MHz
#pragma config FNOSC = FRCPLL, POSCMOD = OFF
#pragma config FPLLIDIV = DIV_2, FPLLMUL = MUL_20, FPLLODIV = DIV_2  //40 MHz
#pragma config FPBDIV = DIV_1 // PB 40 MHz
#pragma config FWDTEN = OFF,  JTAGEN = OFF
#pragma config FSOSCEN = OFF  //PINS 11 and 12 to secondary oscillator!
#pragma config DEBUG = OFF   // RB4 and RB5
//==============================================================
// Protothreads configure

// IF use_vref_debug IS defined, pin 25 is Vref output
//#define use_vref_debug

// IF use_uart_serial IS defined, pin 21 and pin 22 are used by the uart
//#define use_uart_serial
#define COMP_BAUDRATE 9600 // must match PC terminal emulator setting
#define ESP_BAUDRATE 115200

/////////////////////////////////
// set up clock parameters
// system cpu clock
#define sys_clock 40000000

// sys_clock/FPBDIV
#define pb_clock sys_clock/1 // divide by one in this case

#define clrscr() printf( "\x1b[2J")
#define home()   printf( "\x1b[H")
#define pcr()    printf( '\r')
#define crlf     putchar(0x0a); putchar(0x0d);
#define backspace 0x7f // make sure your backspace matches this!

#define cursor_pos(line,column) printf("\x1b[%02d;%02dH",line,column)	
#define clr_right printf("\x1b[K")
// from http://www.comptechdoc.org/os/linux/howlinuxworks/linux_hlvt100.html
// and colors from http://www.termsys.demon.co.uk/vtansi.htm
#define green_text printf("\x1b[32m")
#define yellow_text printf("\x1b[33m")
#define red_text printf("\x1b[31m")
#define rev_text printf("\x1b[7m")
#define normal_text printf("\x1b[0m")

int main(void) {
  
  SYSTEMConfig(sys_clock, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

  ANSELA =0; //make sure analog is cleared
  ANSELB =0;
  
  // === init the uart2 ===================
  // SET UART i/o PINS
  // The RX pin must be one of the Group 2 input pins
  // RPA1, RPB1, RPB5, RPB8, RPB11
  PPSInput (2, U2RX, RPA1); //Assign U2RX to pin RPA1 -- 
  // The TX pin must be one of the Group 4 output pins
  // RPA3, RPB0, RPB9, RPB10, RPB14 
  PPSOutput(4, RPB10, U2TX); //Assign U2TX to pin RPB10 -- 

  UARTConfigure(UART2, UART_ENABLE_PINS_TX_RX_ONLY);
  UARTSetLineControl(UART2, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
  UARTSetDataRate(UART2, pb_clock, COMP_BAUDRATE);
  UARTEnable(UART2, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
  
  // === init the uart1 ===================
  // SET UART i/o PINS
  PPSInput (3, U1RX, RPA2); //Assign U1RX to pin RPA2 -- 
  PPSOutput(1, RPB7, U1TX); //Assign U2TX to pin RPB10 -- 
 
  UARTConfigure(UART1, UART_ENABLE_PINS_TX_RX_ONLY);
  UARTSetLineControl(UART1, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
  UARTSetDataRate(UART1, pb_clock, ESP_BAUDRATE);
  UARTEnable(UART1, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
  
  // set Key pin, default to high (AT mode)
//  mPORTBSetPinsDigitalOut(BIT_8);
//  mPORTBClearBits(BIT_8);
//  mPORTBSetBits(BIT_8);
  
  char buffer[128];
  int bptr = 0;

  while(1) {
    // check if char available from computer
    if(UARTReceivedDataIsAvailable(UART2)) {
      char character = UARTGetDataByte(UART2);
      while(!UARTTransmitterIsReady(UART2));
      UARTSendDataByte(UART2, character);
      buffer[bptr++] = character;
      if(character == '\r'){
        while(!UARTTransmitterIsReady(UART2));
        UARTSendDataByte(UART2, '\n');
//        if (buffer[0] == 'd' && buffer[1] == 'a' && buffer[2] == 't' && buffer[3] == '\r') {
//          // set to low: data mode
//          mPORTBClearBits(BIT_8);
//        } else if (buffer[0] == 'c' && buffer[1] == 'm' && buffer[2] == 'd' && buffer[3] == '\r') {
//          // set to high: command mode
//          mPORTBSetBits(BIT_8);
//        } else {        
          buffer[bptr++] = '\n';

          int i;
          for(i = 0; i < bptr; i++) {
            // send to the computer
            while(!UARTTransmitterIsReady(UART1));
            UARTSendDataByte(UART1, buffer[i]); 
//          }          
        }
        // reset buffer
        bptr = 0; 
      }
    }
    
    // check if char available from bluetooth
    if(UARTReceivedDataIsAvailable(UART1)) {
      char character = UARTGetDataByte(UART1);
      // send to the computer
      while(!UARTTransmitterIsReady(UART2));
      UARTSendDataByte(UART2, character);
    }
  }
}

