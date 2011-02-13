// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

#define F_CPU 14745600

#include <stdlib.h>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>

#include "../libnerdkits/lcd.h"

// pin definitions for buttons
#define B_LEFT   (1<<PC0)
#define B_DOWN   (1<<PC1)
#define B_UP     (1<<PC2)
#define B_RIGHT  (1<<PC3)
#define B_SELECT (1<<PC4)

// size of the game board
#define MAX_WIDTH 20
#define MAX_HEIGHT 4

// living state for the button reader
struct button_states {
    // the stable state (repeated agreeing reads) of the buttons
    uint8_t stable;
    // the most recent read of the buttons
    uint8_t last_read;
};

struct game_state {
    // size of the board
    int width, height;
    // number of unique piece types
    int variety;
    // board state (with enough room for the larges board)
    char board[MAX_HEIGHT][MAX_WIDTH];
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

char random_piece(struct game_state game) {
    return 'a'+(rand() % game.variety);
}

// initialize the board
void boot_board(struct game_state* game) {
    int r,c;
    for (r = 0; r < game->height; r++)
        for (c = 0; c < game->width; c++)
            game->board[r][c] = ' ';
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
    // 14740000/1024 = 14395
    // 14395/60 = 240 (60Hz being target frame rate)
    // choose clock source as system/prescaler1024
    TCCR0B |= (1<<CS02) | (1<<CS00);

    // choose the value for Output Compare A
    OCR0A = 240;
  
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

// handle any directional button pushes
void move_cursor(uint8_t buttons_pushed,
                 struct point* cursor,
                 struct game_state game) {
    if (buttons_pushed & B_UP)
        cursor->row--;
    if (buttons_pushed & B_DOWN)
        cursor->row++;
    if (buttons_pushed & B_LEFT)
        cursor->column--;
    if (buttons_pushed & B_RIGHT)
        cursor->column++;

    if (cursor->row > (game.height-1))
        cursor->row = 0;
    else if (cursor->row < 0)
        cursor->row = (game.height-1);

    if (cursor->column > (game.width-1))
        cursor->column = 0;
    else if (cursor->column < 0)
        cursor->column = (game.width-1);
}

// return true if the given columns/rows are neighbors
uint8_t are_neighbor_rowcols(int rc1, int rc2, int max) {
    int diff = rc1 - rc2;
    return ((diff == 1) ||     // p1 is right-of/below p2
            (-diff == 1) ||    // p1 is left-of/above of p2
            (diff == max-1) || // p1 is far right/bottom, p2 is far left/top
            (-diff == max-1)); // p1 is far left/top, p2 is far right/bottom
}

// return true if the given points are neighbors
uint8_t are_neighbors(struct point p1, struct point p2,
                      struct game_state game) {
    if (p1.row == p2.row)
        return are_neighbor_rowcols(p1.column, p2.column, game.width);
    else if (p1.column == p2.column)
        return are_neighbor_rowcols(p1.row, p2.row, game.height);
    return 0;
}

void invalidate_selection(struct point* selection) {
    selection->meta = 0;
}

uint8_t selection_is_active(struct point selection) {
    return selection.meta != 0;
}

void clear_selection(struct game_state* game, struct point* selection) {
    if (selection_is_active(*selection))
        game->board[selection->row][selection->column] |= 0x20;
    invalidate_selection(selection);
}

void set_selection(struct game_state* game,
                   struct point* selection,
                   struct point cursor) {
    selection->row = cursor.row;
    selection->column = cursor.column;
    selection->meta = 1;
    game->board[selection->row][selection->column] &= ~0x20;
}

// return the index to the row to the "right" of the given row
int next_row(int r, struct game_state game) {
    if (++r > (game.height-1)) return 0;
    return r;
}

// return the index to the column "below" the given column
int next_column(int c, struct game_state game) {
    if (++c > (game.width-1)) return 0;
    return c;
}

// determine if a, b, and c are the same piece
int match(char a, char b, char c) {
    return ((0x0F & b) == (0x0F & a)) && ((0x0F & b) == (0x0F & c));
}

// mark all sets on the board (as capital letters)
int mark_sets(struct game_state* game) {
    int r, nr, nnr, c, nc, nnc, found=0;
    for(r=0, nr=next_row(r, *game), nnr=next_row(nr, *game);
        r < game->height;
        r++, nr=next_row(nr, *game), nnr=next_row(nnr, *game)) {
        for(c=0, nc=next_column(c, *game), nnc=next_column(nc, *game);
            c < game->width;
            c++, nc=next_column(nc, *game), nnc=next_column(nnc, *game)) {
            if(match(game->board[r][c],
                     game->board[r][nc],
                     game->board[r][nnc])) {
                found = 1;
                game->board[r][c] &= ~0x20;
                game->board[r][nc] &= ~0x20;
                game->board[r][nnc] &= ~0x20;
            }
            if(match(game->board[r][c],
                     game->board[nr][c],
                     game->board[nnr][c])) {
                found = 1;
                game->board[r][c] &= ~0x20;
                game->board[nr][c] &= ~0x20;
                game->board[nnr][c] &= ~0x20;
            }
        }
    }
    return found;
}

// remove all sets on the board (as previously marked)
void remove_sets(struct game_state* game) {
    int r, c;
    for(r=0; r < game->height; r++) {
        for(c=0; c < game->width; c++) {
            if ((game->board[r][c] & 0x20) == 0)
                game->board[r][c] = ' ';
        }
    }
}

// move the piece at a to position b, and the piece at b to positiona
void swap_pieces(struct game_state* game,
                 struct point a, struct point b) {
    char p = game->board[a.row][a.column];
    game->board[a.row][a.column] = game->board[b.row][b.column];
    game->board[b.row][b.column] = p;
}

// handle a select button push
uint8_t do_select(uint8_t buttons_pushed,
                  struct game_state* game,
                  struct point cursor,
                  struct point *selection) {
    if (buttons_pushed & B_SELECT) {
        if (selection_is_active(*selection)) {
            if (are_neighbors(*selection, cursor, *game)) {
                clear_selection(game, selection);
                swap_pieces(game, *selection, cursor);
                if (mark_sets(game))
                    return 1;
                swap_pieces(game, *selection, cursor);
            } else {
                clear_selection(game, selection);
                if (cursor.row != selection->row ||
                    cursor.column != selection->column)
                    set_selection(game, selection, cursor);
            }
        } else {
            set_selection(game, selection, cursor);
        }
    }
    return 0;
}

void write_board(struct game_state game) {
    int r, c;
    for (r=0; r < game.height; r++) {
        lcd_goto_position(r, 0);
        for (c=0; c < game.width; c++) {
            lcd_write_data(game.board[r][c]);
        }
    }
}

void clear_blink_state(struct blink_state* blink_state) {
    blink_state->counter = 0;
    blink_state->blink = 0;
}

int find_first_space(char* row, int width) {
    int c;
    for (c = 0; c < width; c++)
        if (row[c] == ' ')
            break;
    return c;
}

void shift(char* row, int width, int start) {
    for (; start < (width-1); start++)
        row[start] = row[start+1];
}

int fill_spaces_row(char* row, struct game_state game) {
    int first_space = find_first_space(row, game.width);
    if (first_space < game.width) {
        shift(row, game.width, first_space);
        row[(game.width-1)] = random_piece(game);
        return 1;
    } else
        return 0;
}

int fill_spaces(struct game_state* game) {
    int r, spaces = 0;
    for(r = 0; r < game->height; r++) {
        spaces |= fill_spaces_row(game->board[r], *game);
    }
    return spaces;
}

void animate_space_fill(struct game_state* game) {
    int spaces = 1, move = 0;
    while(spaces) {
        if (animate) {
            animate = 0;
            if(move > 14) {
                move = 0;
                spaces = fill_spaces(game);
                write_board(*game);
            } else
                move++;
        }
    }
}

void maybe_blink(struct blink_state* blink_state,
                 struct game_state game,
                 struct point cursor) {
    // 30 should be a half second, if we run at 60 Hz
    if (blink_state->counter > 30) {
        lcd_goto_position(cursor.row, cursor.column);
        lcd_write_data(blink_state->blink ?
                       game.board[cursor.row][cursor.column] : 0xA5);
        blink_state->blink = ~blink_state->blink;
        blink_state->counter = 0;
    } else {
        blink_state->counter++;
    }
}

void animate_clear_sets(struct game_state* game) {
    do {
        remove_sets(game);
        write_board(*game);
        animate_space_fill(game);
    } while (mark_sets(game));
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
    int8_t i;
 
    // 'seed' is the value we are going to generate.
    // Starting with zeros in all 16 bits of the 16-bit unsigned integer,
    // we XOR the bits one by one with a highly volatile bit value from ADC,
    // and do it 100 times to mix things up really well.
    for (i = 0; i < 100; i++) {
        // XOR the seed with the random bit from ADC shifted
        seed ^= (adc_get_next_bit() << (i%16));
    }
    return seed;
}

void clear_marks(struct game_state* game) {
    int r, c;
    for (r = 0; r < game->height; r++)
        for (c = 0; c < game->width; c++)
            game->board[r][c] |= 0x20;
}

int8_t valid_move(struct game_state game,
                  struct point a, struct point b) {
    int8_t valid = 0;
    swap_pieces(&game, a, b);
    if (mark_sets(&game)) {
        valid = 1;
        clear_marks(&game);
    }
    swap_pieces(&game, a, b);
    return valid;
}

int8_t valid_move_exists(struct game_state game) {
    struct point check, right, below;
    int8_t valid = 0;
    for (check.row = 0, right.row = 0, below.row = 1;
         check.row < game.height;
         check.row++, right.row++, below.row=next_row(below.row, game)) {
        for (check.column = 0, right.column = 1, below.column = 0;
             !valid && check.column < game.width;
             check.column++, right.column=next_column(right.column, game),
                 below.column++) {
            valid = valid_move(game, check, right) ||
                valid_move(game, check, below);
        }
    }
    return valid;
}

void fb_prompt(int row, char left, char right) {
    lcd_goto_position(row, 10);
    lcd_write_data(left);
    lcd_goto_position(row, 16);
    lcd_write_data(right);
}
void focus_prompt(int row) {
    fb_prompt(row, 0x7e, 0x7f);
}

void blur_prompt(int row) {
    fb_prompt(row, ' ', ' ');
}

void write_prompt(int row, int val) {
    lcd_goto_position(row, 12);
    if (val < 10)
        lcd_write_data(' ');
    lcd_write_int16(val);
}

void prompt_params(struct game_state* game) {
    int ready = 0, prompt = 0;
    uint8_t pressed_buttons;
    int *field, min, max;
    struct button_states button_state;
    clear_button_state(&button_state);
    lcd_clear_and_home();

    lcd_goto_position(0, 3);
    lcd_write_string(PSTR("width:"));
    write_prompt(0, game->width);
    focus_prompt(0);

    lcd_goto_position(1, 2);
    lcd_write_string(PSTR("height:"));
    write_prompt(1, game->height);

    lcd_goto_position(2, 1);
    lcd_write_string(PSTR("variety:"));
    write_prompt(2, game->variety);

    lcd_goto_position(3, 11);
    lcd_write_string(PSTR("start"));

    while(!ready) {
        if (animate) {
            animate = 0;

            pressed_buttons = read_buttons(&button_state);
            if(pressed_buttons) {
                if (pressed_buttons & B_SELECT && prompt == 3) {
                    ready = 1;
                } else if (pressed_buttons & (B_UP | B_DOWN | B_SELECT)) {
                    blur_prompt(prompt);
                    if (pressed_buttons & B_UP) {
                        prompt--;
                        if (prompt < 0) prompt = 3;
                    } else {
                        prompt++;
                        if (prompt > 3) prompt = 0;
                    }
                    focus_prompt(prompt);
                } else {
                    switch (prompt) {
                    case 0: field = &game->width;
                        min=10; max=MAX_WIDTH;
                        break;
                    case 1: field = &game->height;
                        min=3; max=MAX_HEIGHT;
                        break;
                    default: field = &game->variety;
                        min=5; max=26;
                    }
                    if (pressed_buttons & B_RIGHT &&
                        *field < max)
                        *field = *field+1;
                    else if (pressed_buttons & B_LEFT &&
                             *field > min)
                        *field = *field-1;
                    write_prompt(prompt, *field);
                }
            }
        }
    }
}

void play_game(struct game_state* game) {
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

    boot_board(game);
    lcd_clear_and_home();
    animate_clear_sets(game);
    int8_t move_exists = valid_move_exists(*game);
    // now let play begin
    while(move_exists) {
        if (animate) {
            animate = 0;
            pressed_buttons = read_buttons(&button_state);

            if(pressed_buttons) {
                move_cursor(pressed_buttons, &cursor, *game);
                if(do_select(pressed_buttons, game, cursor, &selection)) {
                    animate_clear_sets(game);
                    move_exists = valid_move_exists(*game);
                }
                write_board(*game);
            }

            maybe_blink(&blink_state, *game, cursor);
        }
    }
}

void show_game_over() {
    int delay = 0;
    // game is over (no more moves)
    lcd_clear_and_home();
    lcd_goto_position(1, 8);
    lcd_write_string(PSTR("GAME"));
    lcd_goto_position(2, 8);
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
    struct game_state game;
    game.width = MAX_WIDTH;
    game.height = MAX_HEIGHT;
    game.variety = 5;

    boot_lcd();
    boot_pins();
    boot_timer();
    boot_adc();
    srand(random_seed_from_ADC());
    sei(); //enable interrupts

    while(1) {
        prompt_params(&game);
        play_game(&game);
        show_game_over();
    }

    return 0;
}
