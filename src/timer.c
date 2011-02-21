// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

// details about the game clock

#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "timer.h"
#include "buttons.h"

// wheter or not the animate timer has clicked
volatile int animatev = 0;

ISR(TIMER0_COMPA_vect) {
    // time to cycle animations
    animatev = 1;
}

// configure the animation timer at boot
void boot_timer() {
    // Clear Timer on Compare Match of OCRA
    TCCR0A |= (1<<WGM01);

    // system clock is ~1.47Mhz
    // 14740000/1024 = 14395
    // 14395/60 = 240 (60Hz being target frame rate)
    // choose clock source as system/prescaler1024
    TCCR0B |= (1<<CS02) | (1<<CS00);

    // choose the value for Output Compare A
    OCR0A = 240;
  
    // endable Timer Output Compare Match A Interrupt 0
    TIMSK0 |= (1<<OCIE0A);
}

uint8_t animate() {
    if (animatev) {
        animatev = 0;
        return 1;
    }
    return 0;
}

void simple_delay(int clicks) {
    struct button_states button_state;
    clear_button_state(&button_state);

    while (clicks > 0) {
        if (animate()) {
            clicks--;
            if (read_buttons(&button_state) & B_SELECT)
                break; // skip duration if Select is pushed
        }
    }
}
