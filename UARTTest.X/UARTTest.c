/* 
 * File:   UARTTest.c
 * Author: dantech
 * 
 * Pin config:
 * RA1 -> USB TX (Green wire)
 * RB10 -> USB RX (White wire)
 * 
 * RA2 -> ESP TX (next to GND)
 * RB7 -> ESP RX (next to VCC)
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

volatile unsigned int time_tick_millsec;

void __ISR(_TIMER_5_VECTOR, IPL2AUTO) Timer5Handler(void) {
    // clear the interrupt flag
    mT5ClearIntFlag();
    //count milliseconds
    time_tick_millsec++ ;
}

#define COMP_BAUDRATE 9600 // must match PC terminal emulator setting
#define ESP_BAUDRATE 115200

#define UART_ESP UART1
#define UART_COMP UART2

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


#define BUFFER_SIZE 1024
static char read_buffer[BUFFER_SIZE];
static int buf_ptr = 0;

// 5 seconds
#define DEFAULT_TIMEOUT 5000

void send_byte(char b, UART_MODULE m) {
  while(!UARTTransmitterIsReady(m));
  UARTSendDataByte(m, b);
}

int ends_with(char* buf, int buf_len, char* term, int term_len) {
  int i;
  int ti = term_len-1;
  for (i = buf_len-1; i >= 0 && ti >= 0; i--, ti--) {
    if (buf[i] != term[ti]) {
      return 1;
    }
  }
  return 0;
}

/**
 * 
 * @param timeout
 * @return 0 if newline seen, 1 if buffer overflowed, 2 if timed out
 */
int get_line(int timeout, UART_MODULE m, char* term, int echo) {
  static int start_time;
  start_time = time_tick_millsec;
  
  int term_len = strlen(term);
  
  buf_ptr = 0;
  while(1) {
    if(UARTReceivedDataIsAvailable(m)) {
      // reset timeout when we get a character
      start_time = time_tick_millsec;
      
      // read the character
      char character = UARTGetDataByte(m);
      // put it in the buffer
      read_buffer[buf_ptr++] = character;
      
      if (echo == 1) {
        // echo the character to the screen
        send_byte(character, m);
      }
      
      // check for termination
      if (ends_with(read_buffer, buf_ptr, term, term_len) == 0) {
        if (echo == 1 && m == UART_COMP) {
          send_byte('\n', m);
        }
        read_buffer[buf_ptr] = '\0';
        return 0;
      }
      
      // check for termination
      if (buf_ptr == BUFFER_SIZE-1) {
        read_buffer[buf_ptr] = '\0';
        return 1;
      }
    }
    if (time_tick_millsec - start_time > timeout) {
      return 2;
    }
  }
}

/**
 * Sends the command cmd with a \r\n to UART module m
 * @param cmd
 * @param m
 */
void send_cmd(char* cmd, UART_MODULE m) {
  while(*cmd != NULL) {
    send_byte(*(cmd++), m);
  }
  send_byte('\r', m);
  send_byte('\n', m);
}

void echo_byte_buff() {
  static int i;
  static char intbuff[32];
  for(i = 0; i < buf_ptr; i++) {
    sprintf(intbuff, "Char: %d", (unsigned int) read_buffer[i]);
    send_cmd(intbuff, UART_COMP);
  }
}

static int most_recent_result = 0;

void echo_buff() {
  static char result_buff[32];
  sprintf(result_buff, "Result: %d", most_recent_result);
  send_cmd(result_buff, UART_COMP);
  send_cmd(read_buffer, UART_COMP);
}

#define SEND_CMD_AND_ECHO(cmd) \
send_cmd(cmd, UART_ESP); \
most_recent_result = get_line(DEFAULT_TIMEOUT, UART_ESP, "OK\r\n", 0); \
if (most_recent_result != 0) return 1; \
echo_buff()

int setup() {
  send_cmd("Initiating connection", UART_COMP);
  
  SEND_CMD_AND_ECHO("AT");
          
  SEND_CMD_AND_ECHO("AT");
          
  SEND_CMD_AND_ECHO("AT+GMR");
  
//  send_cmd("AT", UART_ESP);
//  most_recent_result = get_line(DEFAULT_TIMEOUT, UART_ESP, "OK\r\n", 0);
//  echo_buff();
//  
//  send_cmd("AT", UART_ESP);
//  most_recent_result = get_line(DEFAULT_TIMEOUT, UART_ESP, "OK\r\n", 0);
//  echo_buff();
//  
//  send_cmd("AT+GMR", UART_ESP);
//  most_recent_result = get_line(DEFAULT_TIMEOUT, UART_ESP, "OK\r\n", 0);
//  echo_buff();
  
  return 0;
}

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
  
  // ===Set up timer5 ======================
  // timer 5: on,  interrupts, internal clock, 
  // set up to count millsec
  OpenTimer5(T5_ON  | T5_SOURCE_INT | T5_PS_1_1 , pb_clock/1000);
  // set up the timer interrupt with a priority of 2
  ConfigIntTimer5(T5_INT_ON | T5_INT_PRIOR_2);
  mT5ClearIntFlag(); // and clear the interrupt flag
  // zero the system time tick
  time_tick_millsec = 0;
  
  // enable system wide interrupts
  INTEnableSystemMultiVectoredInt();

  if (setup() == 0) {
    send_cmd("Successfully finished setup", UART_COMP); 
  } else {
    send_cmd("Setup failed", UART_COMP); 
  }

  while(1) {
//    if (get_line(DEFAULT_TIMEOUT, UART_COMP) == 2) {
//      send_cmd("Timeout", UART_COMP);
//    }
//    if (strcmp(read_buffer, "abc\r") == 0) {
//      send_cmd("Got some juice", UART_COMP);
//    }
    // check if char available from computer
//    if(UARTReceivedDataIsAvailable()) {
//      char character = UARTGetDataByte(UART2);
//      while(!UARTTransmitterIsReady(UART2));
//      UARTSendDataByte(UART2, character);
//      buffer[bptr++] = character;
//      if(character == '\r'){
//        while(!UARTTransmitterIsReady(UART2));
//        UARTSendDataByte(UART2, '\n');
////        if (buffer[0] == 'd' && buffer[1] == 'a' && buffer[2] == 't' && buffer[3] == '\r') {
////          // set to low: data mode
////          mPORTBClearBits(BIT_8);
////        } else if (buffer[0] == 'c' && buffer[1] == 'm' && buffer[2] == 'd' && buffer[3] == '\r') {
////          // set to high: command mode
////          mPORTBSetBits(BIT_8);
////        } else {        
//          buffer[bptr++] = '\n';
//
//          int i;
//          for(i = 0; i < bptr; i++) {
//            // send to the computer
//            while(!UARTTransmitterIsReady(UART1));
//            UARTSendDataByte(UART1, buffer[i]); 
////          }          
//        }
//        // reset buffer
//        bptr = 0; 
//      }
//    }
    
    // check if char available from bluetooth
//    if(UARTReceivedDataIsAvailable(UART1)) {
//      char character = UARTGetDataByte(UART1);
//      // send to the computer
//      while(!UARTTransmitterIsReady(UART2));
//      UARTSendDataByte(UART2, character);
//    }
  }
}

