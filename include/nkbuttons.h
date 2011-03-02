#ifndef __NKBUTTONS_H__
#define __NKBUTTONS_H__

// pin definitions for buttons
#define B_LEFT   (1<<PC0)
#define B_DOWN   (1<<PC1)
#define B_UP     (1<<PC2)
#define B_RIGHT  (1<<PC3)
#define B_SELECT (1<<PC4)

#define B_LEFT_INT   (1<<PCINT8)
#define B_DOWN_INT   (1<<PCINT9)
#define B_UP_INT     (1<<PCINT10)
#define B_RIGHT_INT  (1<<PCINT11)
#define B_SELECT_INT (1<<PCINT12)

// living state for the button reader
typedef struct {
    // the stable state (repeated agreeing reads) of the buttons
    uint8_t stable;
    // the most recent read of the buttons
    uint8_t last_read;
    // number of times stable state has been read
    uint8_t stable_count;
    // whether or not key triggers are currently repeats
    // (smaller delay between firings after first repeat)
    uint8_t is_repeat;
} nkbuttons_t;

void nkbuttons_init();
void nkbuttons_enable_interrupts();
void nkbuttons_disable_interrupts();
uint8_t nkbuttons_read(nkbuttons_t *state);
void nkbuttons_clear(nkbuttons_t *state);

#endif
