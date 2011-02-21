// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

// utilities for reading button states

#include <avr/pgmspace.h>

#include "buttons.h"

// get the input pins setup at boot
void boot_pins() {
    // Set the 6 pins to input mode - four directions + select
    DDRC &= ~(B_LEFT|B_DOWN|B_UP|B_RIGHT|B_SELECT);
	  
    // turn on the internal resistors for the pins
    PORTC |= (B_LEFT|B_DOWN|B_UP|B_RIGHT|B_SELECT);
}

// check the state of the buttons, returns a mask of
// what buttons are now pushed that weren't before
uint8_t read_buttons(struct button_states* state) {
    // get a fresh read
    uint8_t fresh = ~PINC & (B_LEFT|B_DOWN|B_UP|B_RIGHT|B_SELECT);

    // factor out bounces by sampling twice before determining
    // what is actually pressed
    uint8_t pressed = state->last_read & fresh;

    // find out what buttons are pushed now that weren't before
    uint8_t newly = (state->stable ^ pressed) & pressed;

    // update state
    state->last_read = fresh;
    state->stable = pressed;

    // reply with newly-pushed buttons
    return newly;
}

// clear out all state for the button reader
void clear_button_state(struct button_states* state) {
    state->stable = 0;
    state->last_read = 0;
    // throw away buttons already pressed
    read_buttons(state);
    read_buttons(state);
}

