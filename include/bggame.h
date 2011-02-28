#ifndef __BGGAME_H__
#define __BGGAME_H__

// maximum size of the game board
#define MAX_WIDTH 20
#define MAX_HEIGHT 4

typedef struct {
    // size of the board
    int width, height;
    // number of unique piece types
    int variety;
    // board state (with enough room for the larges board)
    char board[MAX_HEIGHT][MAX_WIDTH];
    // score
    uint16_t score;
} game_t;

// generic "point on the board" structure
typedef struct {
    int8_t row;
    int8_t column;
    // extra data about this point
    // (used for "activeness" of selection)
    int8_t meta;
} point_t;

char bggame_random_piece(game_t game);
void bggame_board_init(game_t *game);
void bggame_move_cursor(game_t game,
                        uint8_t buttons_pushed,
                        point_t* cursor);
uint8_t bggame_are_neighbor_rowcols(int rc1, int rc2, int max);
uint8_t bggame_are_neighbors(game_t game,
                             point_t p1,
                             point_t p2);
void bggame_invalidate_selection(point_t* selection);
uint8_t bggame_selection_is_active(point_t selection);
void bggame_clear_selection(game_t *game, point_t* selection);
void bggame_set_selection(game_t *game,
                          point_t* selection,
                          point_t cursor);
int bggame_next_row(game_t game, int r);
int bggame_next_column(game_t game, int c);
int bggame_match(char a, char b, char c);
int bggame_mark_sets(game_t *game);
uint8_t bggame_remove_sets(game_t *game);
void bggame_swap_pieces(game_t *game, point_t a, point_t b);
uint8_t bggame_select(game_t *game,
                      uint8_t buttons_pushed,
                      point_t cursor,
                      point_t *selection);
void bggame_write_board(game_t game);
int bggame_first_space(char* row, int width);
void bggame_shift(char* row, int width, int start);
int bggame_fill_spaces_row(game_t game, char* row);
int bggame_fill_spaces(game_t *game);
void bggame_animate_space_fill(game_t *game);
void bggame_animate_clear_sets(game_t *game);
void bggame_clear_marks(game_t *game);
int8_t bggame_valid_move(game_t game, point_t a, point_t b);
int8_t bggame_valid_move_exists(game_t game);
void bggame_play(game_t *game);
void bggame_over(uint16_t score);
#endif
