// blockgame
// for NerdKits with ATmega168

#define F_CPU 14745600

#include <stdio.h>
#include <stdlib.h>
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

// size of the game board
#define WIDTH 20
#define HEIGHT 4

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

// generic "point on the board" structure
struct point {
    int8_t row;
    int8_t column;
    // extra data about this point
    // (used for "activeness" of selection)
    int8_t meta;
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

char random_piece() {
    return 'a'+(rand() % 5);
}

// initialize the board
void boot_board(char board[HEIGHT][WIDTH]) {
    int r,c;
    for (r = 0; r < HEIGHT; r++)
        for (c = 0; c < WIDTH; c++)
            board[r][c] = random_piece();
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
void move_cursor(uint8_t buttons_pushed,
                 struct point* cursor) {
    if (buttons_pushed & B_UP)
        cursor->row--;
    if (buttons_pushed & B_DOWN)
        cursor->row++;
    if (buttons_pushed & B_LEFT)
        cursor->column--;
    if (buttons_pushed & B_RIGHT)
        cursor->column++;

    if (cursor->row > (HEIGHT-1))      cursor->row = 0;
    else if (cursor->row < 0) cursor->row = (HEIGHT-1);

    if (cursor->column > (WIDTH-1))      cursor->column = 0;
    else if (cursor->column < 0) cursor->column = (WIDTH-1);
}

uint8_t are_neighbors(struct point p1, struct point p2) {
    if (p1.row == p2.row) {
        switch (p1.column - p2.column) {
        case 1: return 1; // p1 is right of p2
        case -1: return 1; // p1 is left of p2
        case -(WIDTH-1): return 1; // p1 is on the left, p2 is on the right
        case (WIDTH-1): return 1; // p1 is on the right, p2 is on the left
        }
    } else if (p1.column == p2.column) {
        lcd_write_data('c');
        switch (p1.row - p2.row) {
        case 1: return 1; // p1 is below p2
        case -1: return 1; // p1 is above p2
        case -(HEIGHT-1): return 1; // p1 is on the top, p2 is on the bottom
        case (HEIGHT-1):  return 1; // p1 is on the bottom, p2 is on the top
        }
    }
    return 0;
}

void invalidate_selection(struct point* selection) {
    selection->meta = 0;
}

uint8_t selection_is_active(struct point selection) {
    return selection.meta != 0;
}

void clear_selection(char board[HEIGHT][WIDTH], struct point* selection) {
    if (selection_is_active(*selection))
        board[selection->row][selection->column] |= 0x20;
    invalidate_selection(selection);
}

void set_selection(char board[HEIGHT][WIDTH],
                   struct point* selection,
                   struct point cursor) {
    selection->row = cursor.row;
    selection->column = cursor.column;
    selection->meta = 1;
    board[selection->row][selection->column] &= ~0x20;
}

// return the index to the row to the "right" of the given row
int next_row(int r) {
    if (++r > (HEIGHT-1)) return 0;
    return r;
}

// return the index to the column "below" the given column
int next_column(int c) {
    if (++c > (WIDTH-1)) return 0;
    return c;
}

// determine if a, b, and c are the same piece
int match(char a, char b, char c) {
    return ((0x0F & b) == (0x0F & a)) && ((0x0F & b) == (0x0F & c));
}

// mark all sets on the board (as capital letters)
int mark_sets(char board[HEIGHT][WIDTH]) {
    int r, c, found=0;
    for(r=0; r < HEIGHT; r++) {
        for(c=0; c < WIDTH; c++) {
            if(match(board[r][c], board[r][next_column(c)],
                     board[r][next_column(next_column(c))])) {
                found = 1;
                board[r][c] &= ~0x20;
                board[r][next_column(c)] &= ~0x20;
                board[r][next_column(next_column(c))] &= ~0x20;
            }
            if(match(board[r][c], board[next_row(r)][c],
                     board[next_row(next_row(r))][c])) {
                found = 1;
                board[r][c] &= ~0x20;
                board[next_row(r)][c] &= ~0x20;
                board[next_row(next_row(r))][c] &= ~0x20;
            }
        }
    }
    return found;
}

// remove all sets on the board (as previously marked)
void remove_sets(char board[HEIGHT][WIDTH]) {
    int r, c;
    for(r=0; r < HEIGHT; r++) {
        for(c=0; c < WIDTH; c++) {
            if ((board[r][c] & 0x20) == 0)
                board[r][c] = ' ';
        }
    }
}

// move the piece at a to position b, and the piece at b to positiona
void swap_pieces(char board[HEIGHT][WIDTH],
                 struct point a, struct point b) {
    char p = board[a.row][a.column];
    board[a.row][a.column] = board[b.row][b.column];
    board[b.row][b.column] = p;
}

// handle a select button push
uint8_t do_select(uint8_t buttons_pushed,
               char board[HEIGHT][WIDTH],
               struct point cursor,
               struct point *selection) {
    if (buttons_pushed & B_SELECT) {
        if (selection_is_active(*selection)) {
            if (are_neighbors(*selection, cursor)) {
                clear_selection(board, selection);
                swap_pieces(board, *selection, cursor);
                if (mark_sets(board))
                    return 1;
                swap_pieces(board, *selection, cursor);
            } else {
                clear_selection(board, selection);
                if (cursor.row != selection->row ||
                    cursor.column != selection->column)
                    set_selection(board, selection, cursor);
            }
        } else {
            set_selection(board, selection, cursor);
        }
    }
        return 0;
}

void write_row(char row[WIDTH]) {
    int c;

    for (c = 0; c < WIDTH; c++)
        lcd_write_data(row[c]);
}

void write_board(char board[HEIGHT][WIDTH]) {
    int r;
    for (r=0; r < HEIGHT; r++) {
        lcd_goto_position(r, 0);
        write_row(board[r]);
    }
}

void clear_blink_state(struct blink_state* blink_state) {
    blink_state->counter = 0;
    blink_state->blink = 0;
}

int find_first_space(char row[WIDTH]) {
    int c;
    for (c = 0; c < WIDTH; c++)
        if (row[c] == ' ')
            break;
    return c;
}

void shift(char row[WIDTH], int start) {
    for (; start < (WIDTH-1); start++)
        row[start] = row[start+1];
}

int fill_spaces_row(char row[WIDTH]) {
    int first_space = find_first_space(row);
    if (first_space < WIDTH) {
        shift(row, first_space);
        row[(WIDTH-1)] = random_piece();
        return 1;
    } else
        return 0;
}

int fill_spaces(char board[HEIGHT][WIDTH]) {
    int r, spaces = 0;
    for(r = 0; r < HEIGHT; r++) {
        spaces |= fill_spaces_row(board[r]);
    }
    return spaces;
}

void animate_space_fill(char board[HEIGHT][WIDTH]) {
    int spaces = 1, move = 0;
    while(spaces) {
        if (animate) {
            animate = 0;
            if(move > 14) {
                move = 0;
                spaces = fill_spaces(board);
                write_board(board);
            } else
                move++;
        }
    }
}

void maybe_blink(struct blink_state* blink_state,
                 char board[HEIGHT][WIDTH],
                 struct point cursor) {
    // 30 should be a half second, if we run at 60 Hz
    if (blink_state->counter > 30) {
        lcd_goto_position(cursor.row, cursor.column);
        lcd_write_data(blink_state->blink ?
                       board[cursor.row][cursor.column] : 0xA5);
        blink_state->blink = ~blink_state->blink;
        blink_state->counter = 0;
    } else {
        blink_state->counter++;
    }
}

void animate_clear_sets(char board[HEIGHT][WIDTH]) {
    do {
        remove_sets(board);
        write_board(board);
        animate_space_fill(board);
    } while (mark_sets(board));
}

void boot_adc() {
  // set analog to digital converter
  // for external reference (5v), single ended input ADC0
  ADMUX = 0;
 
  // set analog to digital converter
  // to be enabled, with a clock prescale of 1/128
  // so that the ADC clock runs at 115.2kHz.
  ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
 
  // fire a conversion just to get the ADC warmed up
  ADCSRA |= (1<<ADSC);
}
 
uint8_t adc_get_next_bit() {
  // Lowest bit from ADC is the one most likely to change due to minute
  // variations in temperature as measured by LM37 and noise in power
  // supply.  Noise is generally a bad thing, but in our case,
  // the more - the better!  This function reads just the lowest bit
  // from ADC and discards the rest.
 
  // Wait until ADSC goes low (conversion completed).
  while (ADCSRA & (1<<ADSC)) { }
 
  // Read ADCL (AD low byte).  This one has the bit that we want.
  // (Mike mentions in tempsensor.c that ADCL has to be read first,
  // ATmega doc PDF page 259).
  uint16_t adc = ADCL;
  // read ADCH anyway to reset for the next conversion
  // do "something" with it to prevent compiler optimization
  adc ^= ADCH;
 
  // Start the next conversion.
  ADCSRA |= (1<<ADSC);
 
  // Return the lowest bit of ADCL.
  return adc & 1;
}
 
// Generate and return a random value.
uint16_t random_seed_from_ADC() {
  uint16_t seed = 0;
  int8_t i, b;
 
  // 'seed' is the value we are going to generate.
  // Starting with zeros in all 16 bits of the 16-bit unsigned integer,
  // we XOR the bits one by one with a highly volatile bit value from ADC,
  // and do it 100 times to mix things up really well.
  for (i = 0; i < 100; i++) {
    for (b = 0; b < 16; b++) {
      // XOR the seed with the random bit from ADC shifted to position b.
      seed ^= (adc_get_next_bit() << b);
    }
  }
  return seed;
}

void clear_marks(char board[HEIGHT][WIDTH]) {
    int r, c;
    for (r = 0; r < HEIGHT; r++)
        for (c = 0; c < WIDTH; c++)
            board[r][c] |= 0x20;
}

int8_t valid_move(char board[HEIGHT][WIDTH],
                  struct point a, struct point b) {
    int8_t valid = 0;
    swap_pieces(board, a, b);
    if (mark_sets(board)) {
        valid = 1;
        clear_marks(board);
    }
    swap_pieces(board, a, b);
    return valid;
}

int8_t valid_move_exists(char board[HEIGHT][WIDTH]) {
    struct point check, right, below;
    int8_t valid = 0;
    for (check.row = 0, right.row = 0, below.row = 1;
         check.row < HEIGHT;
         check.row++, right.row++, below.row=next_row(below.row)) {
        for (check.column = 0, right.column = 1, below.column = 0;
             !valid && check.column < WIDTH;
             check.column++, right.column=next_column(right.column),
                 below.column++) {
            valid = valid_move(board, check, right) ||
                valid_move(board, check, below);
        }
    }
    return valid;
}

// create a fresh board to play
void create_board(char board[HEIGHT][WIDTH]) {
    int8_t delay = 0;
    boot_board(board);
    write_board(board);
    while(delay < 30) {
        if (animate) {
            animate = 0;
            // show the initial board for a moment
            delay++;
        }
    }
    // then remove initial sets
    animate_clear_sets(board);
}

void play_game(char board[HEIGHT][WIDTH]) {
    // row and column of the cursor
    struct point cursor;
    // read state
    struct button_states button_state;
    // latest new presses
    uint8_t pressed_buttons;
    // blink state
    struct blink_state blink_state;
    // selection state
    struct point selection;

    clear_button_state(&button_state);
    clear_blink_state(&blink_state);
    cursor.row = 0;
    cursor.column = 0;
    invalidate_selection(&selection);

    int8_t move_exists = valid_move_exists(board);
    // now let play begin
    while(move_exists) {
        if (animate) {
            animate = 0;
            pressed_buttons = read_buttons(&button_state);

            if(pressed_buttons) {
                move_cursor(pressed_buttons, &cursor);
                if(do_select(pressed_buttons, board, cursor, &selection)) {
                    animate_clear_sets(board);
                    move_exists = valid_move_exists(board);
                }
                write_board(board);
            }

            maybe_blink(&blink_state, board, cursor);
        }
    }
}

void show_game_over() {
    int8_t delay = 0;
    // game is over (no more moves)
    lcd_clear_and_home();
    lcd_goto_position(2, 7);
    lcd_write_string(PSTR("GAME"));
    lcd_goto_position(3, 7);
    lcd_write_string(PSTR("OVER"));
    while(delay < 300) {
        if (animate) {
            animate = 0;
            // show game over for about 5 seconds
            delay++;
        }
    }
}

int main() {
    // the playing board
    char board[HEIGHT][WIDTH];

    boot_lcd();
    boot_pins();
    boot_timer();
    boot_adc();
    srand(random_seed_from_ADC());
    sei(); //enable interrupts

    while(1) {
        create_board(board);
        play_game(board);
        show_game_over();
    }

    return 0;
}
