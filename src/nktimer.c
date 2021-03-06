// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

// details about the game clock

#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "nktimer.h"
#include "nkbuttons.h"

// wheter or not the animate timer has clicked
volatile int animatev = 0;

ISR(TIMER0_COMPA_vect) {
    // time to cycle animations
    animatev = 1;
}

// configure the animation timer at boot
void nktimer_init(int8_t freq) {
    // Clear Timer on Compare Match of OCRA
    TCCR0A |= (1<<WGM01);

    // system clock is ~1.47Mhz
    // 14740000/1024 = 14395
    // choose clock source as system/prescaler1024
    TCCR0B |= (1<<CS02) | (1<<CS00);

    // for example, 60Hz target:
    // 14395/60 = 240
    // choose the value for Output Compare A
    OCR0A = (F_CPU / 1024) / freq;
  
    nktimer_resume();
}

void nktimer_resume() {
    // endable Timer Output Compare Match A Interrupt 0
    TIMSK0 |= (1<<OCIE0A);
}

void nktimer_pause() {
    // disable Timer Output Compare Match A Interrupt 0
    TIMSK0 &= ~(1<<OCIE0A);
}    

uint8_t nktimer_animate() {
    if (animatev) {
        animatev = 0;
        return 1;
    }
    return 0;
}

void nktimer_simple_delay(int16_t clicks) {
    nkbuttons_t button_state;
    nkbuttons_clear(&button_state);

    while (clicks > 0) {
        if (nktimer_animate()) {
            clicks--;
            if (nkbuttons_read(&button_state) & B_SELECT)
                break; // skip duration if Select is pushed
        }
    }
}
