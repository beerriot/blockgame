// blockgame
// for NerdKits with ATmega168

#define F_CPU 14745600

#include <stdio.h>
#include <math.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>

#include "../libnerdkits/delay.h"
#include "../libnerdkits/lcd.h"
#include "../libnerdkits/uart.h"

// pin definitions for buttons
#define B_LEFT   (1<<PC0)
#define B_DOWN   (1<<PC1)
#define B_UP     (1<<PC2)
#define B_RIGHT  (1<<PC3)
#define B_SELECT (1<<PC5)

#define B_LEFT_INT   (1<<PCINT8)
#define B_DOWN_INT   (1<<PCINT9)
#define B_UP_INT     (1<<PCINT10)
#define B_RIGHT_INT  (1<<PCINT11)
#define B_SELECT_INT (1<<PCINT13)

// living state for the button reader
struct button_states {
    // the stable state (repeated agreeing reads) of the buttons
    uint8_t stable;
    // the most recent read of the buttons
    uint8_t last_read;
};

// living state for the cursor blinker
struct blink_state {
    // the clock counter
    uint8_t counter;
    // which blink are we on?
    uint8_t blink;
};

// wheter or not the animate timer has clicked
volatile int animate = 0;

ISR(TIMER0_COMPA_vect) {
    // time to cycle animations
    animate = 1;
}

// get the LCD setup at boot
void boot_lcd() {
    lcd_init();
    lcd_home();
}

// initialize the board
void boot_board(char board[3][10]) {
    int c;
    for (c = 0; c < 10; c+=3) {
        board[0][c] = 'a';
        board[0][c+1] = 'b';
        board[0][c+2] = 'c';
        board[1][c] = 'c';
        board[1][c+1] = 'a';
        board[1][c+2] = 'b';
        board[2][c] = 'b';
        board[2][c+1] = 'c';
        board[2][c+2] = 'a';
    }
}

// get the input pins setup at boot
void boot_pins() {
    // Set the 6 pins to input mode - four directions + select
    DDRC &= ~(B_LEFT|B_DOWN|B_UP|B_RIGHT|B_SELECT);
	  
    // turn on the internal resistors for the pins
    PORTC |= (B_LEFT|B_DOWN|B_UP|B_RIGHT|B_SELECT);
}

// configure the animation timer at boot
void boot_timer() {
  // Clear Timer on Compare Match of OCRA
  TCCR0A |= (1<<WGM01);

  // system clock is ~1.47Mhz
  // 1470000/1024 = 1435
  // 1435/30 = 212 (30Hz being target frame rate)
  // choose clock source as system/prescaler1024
  TCCR0B |= (1<<CS02) | (1<<CS00);

  // choose the value for Output Compare A
  OCR0A = 212;
  
  // endable Timer Output Compare Match A Interrupt 0
  TIMSK0 |= (1<<OCIE0A);
}

// clear out all state for the button reader
void clear_button_state(struct button_states* state) {
    state->stable = 0;
    state->last_read = 0;
}

// check the state of the buttons, returns a mask of
// what buttons are now pushed that weren't before
uint8_t read_buttons(struct button_states* state) {
    // get a fresh read
    uint8_t fresh = PINC & (B_LEFT|B_DOWN|B_UP|B_RIGHT|B_SELECT);

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

// handle any directional button pushes
void move_cursor(uint8_t buttons_pushed, int* row, int* column) {
    if (buttons_pushed & B_UP)
        (*row)--;
    if (buttons_pushed & B_DOWN)
        (*row)++;
    if (buttons_pushed & B_LEFT)
        (*column)--;
    if (buttons_pushed & B_RIGHT)
        (*column)++;

    if (*row > 2)      *row = 0;
    else if (*row < 0) *row = 2;

    if (*column > 9)      *column = 0;
    else if (*column < 0) *column = 9;
}

// handle a select button push
void do_select(uint8_t buttons_pushed,
               char board[3][10],
               int row, int column) {
    if (buttons_pushed & B_SELECT) {
        if (board[row][column] >= 'a')
            board[row][column] = board[row][column]-('a'-'A');
        else
            board[row][column] = board[row][column]+('a'-'A');
    }
}

void write_row(char row[10]) {
    int c;

    for (c = 0; c < 10; c++)
        lcd_write_data(row[c]);
}

void write_board(char board[3][10]) {
    lcd_home();
    write_row(board[0]);
    lcd_line_two();
    write_row(board[1]);
    lcd_line_three();
    write_row(board[2]);
}

void clear_blink_state(struct blink_state* blink_state) {
    blink_state->counter = 0;
    blink_state->blink = 0;
}

void maybe_blink(struct blink_state* blink_state,
                 char board[3][10],
                 int row, int column) {
    // 30 should be a half second, if we run at 60 Hz
    if (blink_state->counter > 30) {
        lcd_goto_position(row, column);
        lcd_write_data(blink_state->blink ? board[row][column] : 0xA5);
        blink_state->blink = ~blink_state->blink;
        blink_state->counter = 0;
    } else {
        blink_state->counter++;
    }
}

void show_button(struct button_states button_state,
                 uint8_t button, char label) {
    lcd_write_data((button_state.stable & button) ? label : ' ');
    lcd_write_data(' ');
}

void show_buttons(struct button_states button_state) {
    lcd_line_four();
    show_button(button_state, B_LEFT,   'L');
    show_button(button_state, B_DOWN,   'D');
    show_button(button_state, B_UP,     'U');
    show_button(button_state, B_RIGHT,  'R');
    show_button(button_state, B_SELECT, 'S');
}

int main() {

    // row and column of the cursor
    int row = 0, column = 0;
    // the playing board
    char board[3][10];
    // read state
    struct button_states button_state;
    // latest new presses
    uint8_t pressed_buttons;
    // blink state
    struct blink_state blink_state;

    boot_board(board);
    clear_button_state(&button_state);
    clear_blink_state(&blink_state);

    boot_lcd();
    boot_pins();
    boot_timer();

    sei(); //enable interrupts
    write_board(board);
    while(1) {
        if (animate) {
            animate = 0;
            pressed_buttons = read_buttons(&button_state);

            if(pressed_buttons) {
                move_cursor(pressed_buttons, &row, &column);
                do_select(pressed_buttons, board, row, column);
                write_board(board);
            }

            maybe_blink(&blink_state, board, row, column);

            show_buttons(button_state); // debug
        }
    }
    return 0;
}
