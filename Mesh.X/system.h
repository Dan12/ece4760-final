/* 
 * File:   system.h
 * Author: dantech
 *
 * Created on November 15, 2018, 5:16 PM
 */

#ifndef SYSTEM_H
#define	SYSTEM_H

/////////////////////////////////
// set up clock parameters
// system cpu clock
#define sys_clock 40000000

// sys_clock/FPBDIV
#define pb_clock sys_clock/1 // divide by one in this case

static void init_system() {

    SYSTEMConfig(sys_clock, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

    ANSELA = 0; //make sure analog is cleared
    ANSELB = 0;
}

#endif	/* SYSTEM_H */

