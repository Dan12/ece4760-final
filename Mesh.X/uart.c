#include "uart.h"
#include "timer.h"
#include "system.h"
#include "string_util.h"

#define DMA_BUFFER_SIZE 2048
volatile char dma_buffer[DMA_BUFFER_SIZE];
volatile unsigned int write_head = 0;
volatile unsigned int read_head = 0;

void __ISR(_DMA1_VECTOR, IPL2AUTO) DMACellTransfered(void) {
  write_head = (write_head + 1) % DMA_BUFFER_SIZE;
  DmaChnClrEvFlags(1, DMA_EV_CELL_DONE);
  INTClearFlag(INT_SOURCE_DMA(DMA_CHANNEL1));
}

#define COMP_BAUDRATE 9600 // must match PC terminal emulator setting
#define ESP_BAUDRATE 115200

void send_byte(char b, UART_MODULE m) {
  while(!UARTTransmitterIsReady(m));
  UARTSendDataByte(m, b);
}

int get_comp_data(int timeout, char* ret_buf, int* len) {
  static int start_time;
  start_time = time_tick_millsec;
  
  int buf_len = *len;
  *len = 0;
  while(1) {
    if(UARTReceivedDataIsAvailable(UART_COMP)) {
      // reset timeout when we get a character
      start_time = time_tick_millsec;
      
      // read the character
      char character = UARTGetDataByte(UART_COMP);
      // put it in the buffer
      ret_buf[(*len)++] = character;
      
      // echo the character to the screen
      send_byte(character, UART_COMP);
      
      // check for termination
      if (character == '\r') {
        send_byte('\n', UART_COMP);
        ret_buf[*len] = '\0';
        return 1;
      }
      
      // check for overrun
      if (*len == buf_len) {
        return 0;
      }
    }
    if (time_tick_millsec - start_time > timeout) {
      return 0;
    }
  }
}

int get_esp_data(int timeout, char* ret_buf, int* len) {
  static int start_time;
  start_time = time_tick_millsec;
  
  int buf_len = *len;
  *len = 0;
  while(1) {
    if (write_head != read_head) {
      // reset timeout when we get a character
      start_time = time_tick_millsec;
      
      // read the character
      char character = dma_buffer[read_head++];
      read_head = read_head % DMA_BUFFER_SIZE;
      
      // put it in the buffer
      ret_buf[(*len)++] = character;

      // check for successful termination
      if (ends_with(ret_buf, *len, "OK\r\n", 4) == 0) {
        ret_buf[*len] = '\0';
        return 1;
      }
      // check for error termination
      if (ends_with(ret_buf, *len, "ERROR\r\n", 7) == 0) {
        return 0;
      }
      
      // check for overrun
      if (*len == buf_len) {
        return 0;
      }
    }
    if (time_tick_millsec - start_time > timeout) {
      return 0;
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

void init_uart() {
  init_timer();
  
  // === init the uart2 for computer ===================
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
  
  // === init the uart1 for ESP ===================
  // SET UART i/o PINS
  PPSInput (3, U1RX, RPA2); //Assign U1RX to pin RPA2 -- 
  PPSOutput(1, RPB7, U1TX); //Assign U2TX to pin RPB10 -- 
 
  UARTConfigure(UART1, UART_ENABLE_PINS_TX_RX_ONLY);
  UARTSetLineControl(UART1, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
  UARTSetDataRate(UART1, pb_clock, ESP_BAUDRATE);
  UARTEnable(UART1, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));
  
  // ===Set up DMA for receiving from comp ======================
  // channel 1, priority 0
  DmaChnOpen(1, 0, DMA_OPEN_AUTO);
  // channel 1
  if (dma_uart_chnl == UART1) {
    DmaChnSetEventControl(1, DMA_EV_START_IRQ_EN | DMA_EV_START_IRQ(_UART1_RX_IRQ));
    DmaChnSetTxfer(1, (void*)&U1RXREG, (void*)dma_buffer, 1, DMA_BUFFER_SIZE, 1); 
  } else {
    DmaChnSetEventControl(1, DMA_EV_START_IRQ_EN | DMA_EV_START_IRQ(_UART2_RX_IRQ));
    DmaChnSetTxfer(1, (void*)&U2RXREG, (void*)dma_buffer, 1, DMA_BUFFER_SIZE, 1); 
  }
  
  // channel 1, cell (byte) done
  DmaChnSetEvEnableFlags(1, DMA_EV_CELL_DONE);
  mDmaChnSetIntPriority(1, 2, 2);
  // enable interrupts on channel 1
  mDmaChnIntEnable(1);
  INTClearFlag(INT_SOURCE_DMA(DMA_CHANNEL1));
  
  DmaChnEnable(1);
}