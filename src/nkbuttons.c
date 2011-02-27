// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

// utilities for reading button states

#include <avr/pgmspace.h>

#include "nkbuttons.h"

// get the input pins setup at boot
void nkbuttons_init() {
    // Set the 6 pins to input mode - four directions + select
    DDRC &= ~(B_LEFT|B_DOWN|B_UP|B_RIGHT|B_SELECT);
	  
    // turn on the internal resistors for the pins
    PORTC |= (B_LEFT|B_DOWN|B_UP|B_RIGHT|B_SELECT);
}

// check the state of the buttons, returns a mask of
// what buttons are now pushed that weren't before
uint8_t nkbuttons_read(struct nkbuttons* state) {
    // get a fresh read
    uint8_t fresh = ~PINC & (B_LEFT|B_DOWN|B_UP|B_RIGHT|B_SELECT);

    // factor out bounces by sampling twice before determining
    // what is actually pressed
    uint8_t pressed = state->last_read & fresh;
    state->last_read = fresh;

    uint8_t newly = 0;
    if (state->stable == pressed) {
        if (++state->stable_count > (state->is_repeat ? 14 : 30)) {
            state->stable_count = 0;
            state->is_repeat = 1;
            // trigger a key repeat, if it's still held down
            newly = pressed;
        }
    } else {
        state->stable_count = 0;
        state->is_repeat = 0;
        // find out what buttons are pushed now that weren't before
        newly = (state->stable ^ pressed) & pressed;
        state->stable = pressed;
    }

    // reply with newly-pushed buttons
    return newly;
}

// clear out all state for the button reader
void nkbuttons_clear(struct nkbuttons* state) {
    state->stable = 0;
    state->last_read = 0;
    state->stable_count = 0;
    state->is_repeat = 0;
    // throw away buttons already pressed
    nkbuttons_read(state);
    nkbuttons_read(state);
}
