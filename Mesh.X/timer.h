/* 
 * File:   timer.h
 * Author: dantech
 *
 * Created on November 15, 2018, 5:31 PM
 */

#define _SUPPRESS_PLIB_WARNING 
#define _DISABLE_OPENADC10_CONFIGPORT_WARNING
#include <plib.h>

#ifndef TIMER_H
#define	TIMER_H

volatile unsigned int time_tick_millsec;

void init_timer();

#endif	/* TIMER_H */

