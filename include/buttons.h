#ifndef __BUTTONS_H__
#define __BUTTONS_H__

// pin definitions for buttons
#define B_LEFT   (1<<PC0)
#define B_DOWN   (1<<PC1)
#define B_UP     (1<<PC2)
#define B_RIGHT  (1<<PC3)
#define B_SELECT (1<<PC4)

// living state for the button reader
struct button_states {
    // the stable state (repeated agreeing reads) of the buttons
    uint8_t stable;
    // the most recent read of the buttons
    uint8_t last_read;
};

void boot_pins();
uint8_t read_buttons(struct button_states* state);
void clear_button_state(struct button_states* state);

#endif
