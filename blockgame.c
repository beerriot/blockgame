// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

#include <stdlib.h>
#include <inttypes.h>

#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "lcd.h" //add nerdkits-provided library

#include "nkbuttons.h"
#include "nkrand.h"
#include "nklcd.h"
#include "nktimer.h"

#include "bghighscore.h"

// size of the game board
#define MAX_WIDTH 20
#define MAX_HEIGHT 4

struct {
    // size of the board
    int width, height;
    // number of unique piece types
    int variety;
    // board state (with enough room for the larges board)
    char board[MAX_HEIGHT][MAX_WIDTH];
} game;

uint16_t score;

// generic "point on the board" structure
struct point {
    int8_t row;
    int8_t column;
    // extra data about this point
    // (used for "activeness" of selection)
    int8_t meta;
};

char random_piece() {
    return 'a'+(rand() % game.variety);
}

// initialize the board
void boot_board() {
    int r,c;
    for (r = 0; r < game.height; r++)
        for (c = 0; c < game.width; c++)
            game.board[r][c] = ' ';
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
uint8_t are_neighbors(struct point p1, struct point p2) {
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

void clear_selection(struct point* selection) {
    if (selection_is_active(*selection))
        game.board[selection->row][selection->column] |= 0x20;
    lcd_goto_position(selection->row, selection->column);
    lcd_write_data(game.board[selection->row][selection->column]);
    invalidate_selection(selection);
}

void set_selection(struct point* selection,
                   struct point cursor) {
    selection->row = cursor.row;
    selection->column = cursor.column;
    selection->meta = 1;
    game.board[selection->row][selection->column] &= ~0x20;
    lcd_goto_position(selection->row, selection->column);
    lcd_write_data(game.board[selection->row][selection->column]);
}

// return the index to the row to the "right" of the given row
int next_row(int r) {
    if (++r > (game.height-1)) return 0;
    return r;
}

// return the index to the column "below" the given column
int next_column(int c) {
    if (++c > (game.width-1)) return 0;
    return c;
}

// determine if a, b, and c are the same piece
int match(char a, char b, char c) {
    return ((0x0F & b) == (0x0F & a)) && ((0x0F & b) == (0x0F & c));
}

// mark all sets on the board (as capital letters)
int mark_sets() {
    int r, nr, nnr, c, nc, nnc, found=0;
    for(r=0, nr=next_row(r), nnr=next_row(nr);
        r < game.height;
        r++, nr=next_row(nr), nnr=next_row(nnr)) {
        for(c=0, nc=next_column(c), nnc=next_column(nc);
            c < game.width;
            c++, nc=next_column(nc), nnc=next_column(nnc)) {
            if(match(game.board[r][c],
                     game.board[r][nc],
                     game.board[r][nnc])) {
                found = 1;
                game.board[r][c] &= ~0x20;
                game.board[r][nc] &= ~0x20;
                game.board[r][nnc] &= ~0x20;
            }
            if(match(game.board[r][c],
                     game.board[nr][c],
                     game.board[nnr][c])) {
                found = 1;
                game.board[r][c] &= ~0x20;
                game.board[nr][c] &= ~0x20;
                game.board[nnr][c] &= ~0x20;
            }
        }
    }
    return found;
}

// remove all sets on the board (as previously marked)
uint8_t remove_sets() {
    int r, c;
    uint8_t removed = 0;
    for(r=0; r < game.height; r++) {
        for(c=0; c < game.width; c++) {
            if ((game.board[r][c] & 0x20) == 0) {
                game.board[r][c] = ' ';
                removed++;
            }
        }
    }
    return removed;
}

// move the piece at a to position b, and the piece at b to positiona
void swap_pieces(struct point a, struct point b) {
    char p = game.board[a.row][a.column];
    game.board[a.row][a.column] = game.board[b.row][b.column];
    game.board[b.row][b.column] = p;
}

// handle a select button push
uint8_t do_select(uint8_t buttons_pushed,
                  struct point cursor,
                  struct point *selection) {
    if (buttons_pushed & B_SELECT) {
        if (selection_is_active(*selection)) {
            if (are_neighbors(*selection, cursor)) {
                clear_selection(selection);
                swap_pieces(*selection, cursor);
                if (mark_sets())
                    return 1;
                swap_pieces(*selection, cursor);
            } else {
                clear_selection(selection);
                if (cursor.row != selection->row ||
                    cursor.column != selection->column)
                    set_selection(selection, cursor);
            }
        } else {
            set_selection(selection, cursor);
        }
    }
    return 0;
}

void write_board() {
    int r, c;
    for (r=0; r < game.height; r++) {
        lcd_goto_position(r, 0);
        for (c=0; c < game.width; c++) {
            lcd_write_data(game.board[r][c]);
        }
    }
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

int fill_spaces_row(char* row) {
    int first_space = find_first_space(row, game.width);
    if (first_space < game.width) {
        shift(row, game.width, first_space);
        row[(game.width-1)] = random_piece();
        return 1;
    } else
        return 0;
}

int fill_spaces() {
    int r, spaces = 0;
    for(r = 0; r < game.height; r++) {
        spaces |= fill_spaces_row(game.board[r]);
    }
    return spaces;
}

void animate_space_fill() {
    int spaces = 1, move = 0;
    while(spaces) {
        if (nktimer_animate()) {
            if(move > 14) {
                move = 0;
                spaces = fill_spaces();
                write_board();
            } else
                move++;
        }
    }
}

void animate_clear_sets() {
    uint8_t combos = 1;
    do {
        score += (combos * remove_sets());
        combos++;
        write_board();
        animate_space_fill();
    } while (mark_sets());
}

void clear_marks() {
    int r, c;
    for (r = 0; r < game.height; r++)
        for (c = 0; c < game.width; c++)
            game.board[r][c] |= 0x20;
}

int8_t valid_move(struct point a, struct point b) {
    int8_t valid = 0;
    swap_pieces(a, b);
    if (mark_sets()) {
        valid = 1;
        clear_marks();
    }
    swap_pieces(a, b);
    return valid;
}

int8_t valid_move_exists() {
    struct point check, right, below;
    int8_t valid = 0;
    for (check.row = 0, right.row = 0, below.row = 1;
         check.row < game.height;
         check.row++, right.row++, below.row=next_row(below.row)) {
        for (check.column = 0, right.column = 1, below.column = 0;
             !valid && check.column < game.width;
             check.column++, right.column=next_column(right.column),
                 below.column++) {
            valid = valid_move(check, right) ||
                valid_move(check, below);
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

void prompt_params() {
    int ready = 0, prompt = 0;
    uint8_t pressed_buttons;
    int *field, min, max;
    struct nkbuttons button_state;
    nkbuttons_clear(&button_state);
    nklcd_stop_blinking();
    lcd_clear_and_home();

    lcd_goto_position(0, 3);
    lcd_write_string(PSTR("width:"));
    write_prompt(0, game.width);
    focus_prompt(0);

    lcd_goto_position(1, 2);
    lcd_write_string(PSTR("height:"));
    write_prompt(1, game.height);

    lcd_goto_position(2, 1);
    lcd_write_string(PSTR("variety:"));
    write_prompt(2, game.variety);

    lcd_goto_position(3, 11);
    lcd_write_string(PSTR("start"));

    while(!ready) {
        if (nktimer_animate()) {
            pressed_buttons = nkbuttons_read(&button_state);
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
                    case 0: field = &game.width;
                        min=10; max=MAX_WIDTH;
                        break;
                    case 1: field = &game.height;
                        min=3; max=MAX_HEIGHT;
                        break;
                    default: field = &game.variety;
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

void play_game() {
    // row and column of the cursor
    struct point cursor;
    // read state
    struct nkbuttons button_state;
    // latest new presses
    uint8_t pressed_buttons;
    // selection state
    struct point selection;

    nkbuttons_clear(&button_state);
    cursor.row = 0;
    cursor.column = 0;
    invalidate_selection(&selection);

    boot_board();
    lcd_clear_and_home();
    animate_clear_sets();
    score = 0; // no points for tiles removed before play starts
    lcd_goto_position(cursor.row, cursor.column);
    nklcd_start_blinking();
    int8_t move_exists = valid_move_exists();
    // now let play begin
    while(move_exists) {
        if (nktimer_animate()) {
            pressed_buttons = nkbuttons_read(&button_state);

            if(pressed_buttons) {
                nklcd_stop_blinking();
                move_cursor(pressed_buttons, &cursor);
                if(do_select(pressed_buttons, cursor, &selection)) {
                    animate_clear_sets();
                    move_exists = valid_move_exists();
                }
                lcd_goto_position(cursor.row, cursor.column);
                nklcd_start_blinking();
            }
        }
    }
}

void show_game_over() {
    // game is over (no more moves)
    nklcd_stop_blinking();
    lcd_clear_and_home();
    lcd_goto_position(0, 6);
    lcd_write_string(PSTR("GAME OVER"));
    lcd_goto_position(2, 4);
    lcd_write_string(PSTR("score: "));
    lcd_write_int16(score);
    nktimer_simple_delay(300);
    
    bghighscore_maybe(score);
    bghighscore_screen();
}

int main() {
    // the playing board
    game.width = MAX_WIDTH;
    game.height = MAX_HEIGHT;
    game.variety = 5;

    nklcd_init();
    nkbuttons_init();
    nktimer_init(60);
    nkrand_init();
    srand(nkrand_seed());
    bghighscore_init();
    sei(); //enable interrupts

    while(1) {
        prompt_params();
        play_game();
        show_game_over();
    }

    return 0;
}
