// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

// logic for actually playing the game

#include <stdlib.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#include "lcd.h"

#include "nkbuttons.h"
#include "nklcd.h"
#include "nktimer.h"
#include "nksleep.h"

#include "bggame.h"
#include "bghighscore.h"

char bggame_random_piece(game_t game) {
    return 'a'+(rand() % game.variety);
}

// initialize the board
void bggame_board_init(game_t *game) {
    int8_t r,c;
    for (r = 0; r < game->height; r++)
        for (c = 0; c < game->width; c++)
            game->board[r][c] = ' ';
}

// handle any directional button pushes
void bggame_move_cursor(game_t game,
                        uint8_t buttons_pushed,
                        point_t *cursor) {
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
uint8_t bggame_are_neighbor_rowcols(int8_t rc1, int8_t rc2, int8_t max) {
    int8_t diff = rc1 - rc2;
    return ((diff == 1) ||     // p1 is right-of/below p2
            (-diff == 1) ||    // p1 is left-of/above of p2
            (diff == max-1) || // p1 is far right/bottom, p2 is far left/top
            (-diff == max-1)); // p1 is far left/top, p2 is far right/bottom
}

// return true if the given points are neighbors
uint8_t bggame_are_neighbors(game_t game,
                             point_t p1,
                             point_t p2) {
    if (p1.row == p2.row)
        return bggame_are_neighbor_rowcols(p1.column, p2.column, game.width);
    else if (p1.column == p2.column)
        return bggame_are_neighbor_rowcols(p1.row, p2.row, game.height);
    return 0;
}

void bggame_invalidate_selection(point_t *selection) {
    selection->meta = 0;
}

uint8_t bggame_selection_is_active(point_t selection) {
    return selection.meta & PM_SELECTED;
}

void bggame_clear_selection(game_t *game, point_t *selection) {
    if (bggame_selection_is_active(*selection))
        game->board[selection->row][selection->column] |= 0x20;
    lcd_goto_position(selection->row, selection->column);
    lcd_write_data(game->board[selection->row][selection->column]);
    bggame_invalidate_selection(selection);
}

void bggame_set_selection(game_t *game,
                          point_t *selection,
                          point_t cursor) {
    selection->row = cursor.row;
    selection->column = cursor.column;
    selection->meta |= PM_SELECTED;
    game->board[selection->row][selection->column] &= ~0x20;
    lcd_goto_position(selection->row, selection->column);
    lcd_write_data(game->board[selection->row][selection->column]);
}

// return the index to the row to the "right" of the given row
int8_t bggame_next_row(game_t game, int8_t r) {
    if (++r > (game.height-1)) return 0;
    return r;
}

// return the index to the column "below" the given column
int8_t bggame_next_column(game_t game, int8_t c) {
    if (++c > (game.width-1)) return 0;
    return c;
}

// determine if a, b, and c are the same piece
uint8_t bggame_match(char a, char b, char c) {
    return ((0x1F & b) == (0x1F & a)) && ((0x1F & b) == (0x1F & c));
}

// mark all sets on the board (as capital letters)
uint8_t bggame_mark_sets(game_t *game) {
    int8_t r, nr, nnr, c, nc, nnc, found=0;
    for(r=0, nr=bggame_next_row(*game, r), nnr=bggame_next_row(*game, nr);
        r < game->height;
        r++, nr=bggame_next_row(*game, nr), nnr=bggame_next_row(*game, nnr)) {
        for(c=0, nc=bggame_next_column(*game, c),
                nnc=bggame_next_column(*game, nc);
            c < game->width;
            c++, nc=bggame_next_column(*game, nc),
                nnc=bggame_next_column(*game, nnc)) {
            if(bggame_match(game->board[r][c],
                            game->board[r][nc],
                            game->board[r][nnc])) {
                found = 1;
                game->board[r][c] &= ~0x20;
                game->board[r][nc] &= ~0x20;
                game->board[r][nnc] &= ~0x20;
            }
            if(bggame_match(game->board[r][c],
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
uint8_t bggame_remove_sets(game_t *game) {
    int8_t r, c;
    uint8_t removed = 0;
    for(r=0; r < game->height; r++) {
        for(c=0; c < game->width; c++) {
            if ((game->board[r][c] & 0x20) == 0) {
                game->board[r][c] = ' ';
                removed++;
            }
        }
    }
    return removed;
}

// move the piece at a to position b, and the piece at b to positiona
void bggame_swap_pieces(game_t *game, point_t a, point_t b) {
    char p = game->board[a.row][a.column];
    game->board[a.row][a.column] = game->board[b.row][b.column];
    game->board[b.row][b.column] = p;
}

// handle a select button push
uint8_t bggame_select(game_t *game,
                      uint8_t buttons_pushed,
                      point_t cursor,
                      point_t *selection) {
    if (buttons_pushed & B_SELECT) {
        if (bggame_selection_is_active(*selection)) {
            if (bggame_are_neighbors(*game, *selection, cursor)) {
                bggame_clear_selection(game, selection);
                bggame_swap_pieces(game, *selection, cursor);
                if (bggame_mark_sets(game))
                    return 1;
                bggame_swap_pieces(game, *selection, cursor);
            } else {
                bggame_clear_selection(game, selection);
                if (cursor.row != selection->row ||
                    cursor.column != selection->column)
                    bggame_set_selection(game, selection, cursor);
            }
        } else {
            bggame_set_selection(game, selection, cursor);
        }
    }
    return 0;
}

void bggame_write_board(game_t game) {
    int8_t r, c;
    for (r=0; r < game.height; r++) {
        lcd_goto_position(r, 0);
        for (c=0; c < game.width; c++) {
            lcd_write_data(game.board[r][c]);
        }
    }
}

int8_t bggame_first_space(char *row, int8_t width) {
    int8_t c;
    for (c = 0; c < width; c++)
        if (row[c] == ' ')
            break;
    return c;
}

void bggame_shift(char *row, int8_t width, int8_t start) {
    for (; start < (width-1); start++)
        row[start] = row[start+1];
}

uint8_t bggame_fill_spaces_row(game_t game, char *row) {
    int8_t first_space = bggame_first_space(row, game.width);
    if (first_space < game.width) {
        bggame_shift(row, game.width, first_space);
        row[(game.width-1)] = bggame_random_piece(game);
        return 1;
    } else
        return 0;
}

uint8_t bggame_fill_spaces(game_t *game) {
    int r, spaces = 0;
    for(r = 0; r < game->height; r++) {
        spaces |= bggame_fill_spaces_row(*game, game->board[r]);
    }
    return spaces;
}

void bggame_animate_space_fill(game_t *game) {
    uint8_t spaces = 1, move = 0;
    while(spaces) {
        if (nktimer_animate()) {
            if(move > 7) {
                move = 0;
                spaces = bggame_fill_spaces(game);
                bggame_write_board(*game);
            } else
                move++;
        }
    }
}

void bggame_animate_clear_sets(game_t *game) {
    uint8_t combos = 1;
    do {
        game->score += (combos * bggame_remove_sets(game));
        combos++;
        bggame_write_board(*game);
        bggame_animate_space_fill(game);
    } while (bggame_mark_sets(game));
}

void bggame_clear_marks(game_t *game) {
    int8_t r, c;
    for (r = 0; r < game->height; r++)
        for (c = 0; c < game->width; c++)
            game->board[r][c] |= 0x20;
}

uint8_t bggame_valid_move(game_t game, point_t a, point_t b) {
    uint8_t valid = 0;
    bggame_swap_pieces(&game, a, b);
    if (bggame_mark_sets(&game)) {
        valid = 1;
        bggame_clear_marks(&game);
    }
    bggame_swap_pieces(&game, a, b);
    return valid;
}

uint8_t bggame_valid_move_exists(game_t game) {
    point_t check, right, below;
    uint8_t valid = 0;
    for (check.row = 0, right.row = 0, below.row = 1;
         check.row < game.height;
         check.row++, right.row++,
             below.row=bggame_next_row(game, below.row)) {
        for (check.column = 0, right.column = 1, below.column = 0;
             !valid && check.column < game.width;
             check.column++,
                 right.column=bggame_next_column(game, right.column),
                 below.column++) {
            valid = bggame_valid_move(game, check, right) ||
                bggame_valid_move(game, check, below);
        }
    }
    return valid;
}

void bggame_play(game_t *game) {
    // row and column of the cursor
    point_t cursor;
    // read state
    nkbuttons_t button_state;
    // latest new presses
    uint8_t pressed_buttons;
    // selection state
    point_t selection;
    // idle/sleep timer
    int16_t idle = 0;

    nkbuttons_clear(&button_state);
    cursor.row = 0;
    cursor.column = 0;
    bggame_invalidate_selection(&selection);

    bggame_board_init(game);
    lcd_clear_and_home();
    bggame_animate_clear_sets(game);
    game->score = 0; // no points for tiles removed before play starts
    lcd_goto_position(cursor.row, cursor.column);
    nklcd_start_blinking();
    uint8_t move_exists = bggame_valid_move_exists(*game);
    // now let play begin
    while(move_exists) {
        if (nktimer_animate()) {
            pressed_buttons = nkbuttons_read(&button_state);

            if(pressed_buttons) {
                idle = 0;
                nklcd_stop_blinking();
                bggame_move_cursor(*game, pressed_buttons, &cursor);
                if(bggame_select(game, pressed_buttons, cursor, &selection)) {
                    bggame_animate_clear_sets(game);
                    move_exists = bggame_valid_move_exists(*game);
                }
                lcd_goto_position(cursor.row, cursor.column);
                nklcd_start_blinking();
            } else if (++idle > 3600) {
                // go to sleep after a minute of no activity
                idle = 0;
                nksleep_standby();
                // display comes out of standby with blink disabled
                nklcd_start_blinking();
            }
        }
    }
}

void bggame_over(uint16_t score) {
    // game is over (no more moves)
    nklcd_stop_blinking();
    lcd_clear_and_home();
    lcd_goto_position(0, 6);
    lcd_write_string(PSTR("GAME OVER"));
    lcd_goto_position(2, 4);
    lcd_write_string(PSTR("score: "));
    lcd_write_int16(score);
    nktimer_simple_delay(300);
}
