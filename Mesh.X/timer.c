#include "timer.h"
#include "system.h"

void __ISR(_TIMER_5_VECTOR, IPL2AUTO) Timer5Handler(void) {
    // clear the interrupt flag
    mT5ClearIntFlag();
    //count milliseconds
    time_tick_millsec++;
}

void init_timer() {
  // ===Set up timer5 ======================
  // timer 5: on,  interrupts, internal clock, 
  // set up to count millsec
  OpenTimer5(T5_ON | T5_SOURCE_INT | T5_PS_1_1, pb_clock / 1000);
  // set up the timer interrupt with a priority of 2
  ConfigIntTimer5(T5_INT_ON | T5_INT_PRIOR_2);
  mT5ClearIntFlag(); // and clear the interrupt flag
  // zero the system time tick
  time_tick_millsec = 0;
}
