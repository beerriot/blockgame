// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

// utilities for reducing power consumption

#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "nksleep.h"
#include "nklcd.h"
#include "nkbuttons.h"
#include "nktimer.h"

void nksleep_standby() {
    nklcd_off();
    nkbuttons_enable_interrupts();
    nktimer_pause();
    // SLEEP
    SMCR = (1<<SM2)|(1<<SM1); //standby
    sleep_enable();
    sleep_cpu();
    sleep_disable();
    // ENDSLEEP
    nktimer_resume();
    nkbuttons_disable_interrupts();
    nklcd_on();
    sei();
}
