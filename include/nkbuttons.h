#ifndef __NKBUTTONS_H__
#define __NKBUTTONS_H__

// pin definitions for buttons
#define B_LEFT   (1<<PC0)
#define B_DOWN   (1<<PC1)
#define B_UP     (1<<PC2)
#define B_RIGHT  (1<<PC3)
#define B_SELECT (1<<PC4)

// living state for the button reader
struct nkbuttons {
    // the stable state (repeated agreeing reads) of the buttons
    uint8_t stable;
    // the most recent read of the buttons
    uint8_t last_read;
};

void nkbuttons_init();
uint8_t nkbuttons_read(struct nkbuttons* state);
void nkbuttons_clear(struct nkbuttons* state);

#endif
