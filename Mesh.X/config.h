/* 
 * File:   config.h
 * Author: dantech
 *
 * Created on November 15, 2018, 5:38 PM
 */

#ifndef CONFIG_H
#define	CONFIG_H

//=============================================================
// 40 MHz
#pragma config FNOSC = FRCPLL, POSCMOD = OFF
#pragma config FPLLIDIV = DIV_2, FPLLMUL = MUL_20, FPLLODIV = DIV_2  //40 MHz
#pragma config FPBDIV = DIV_1 // PB 40 MHz
#pragma config FWDTEN = OFF,  JTAGEN = OFF
#pragma config FSOSCEN = OFF  //PINS 11 and 12 to secondary oscillator!
#pragma config DEBUG = OFF   // RB4 and RB5
//==============================================================

#endif	/* CONFIG_H */

