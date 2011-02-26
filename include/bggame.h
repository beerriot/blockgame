#ifndef __BGGAME_H__
#define __BGGAME_H__

// maximum size of the game board
#define MAX_WIDTH 20
#define MAX_HEIGHT 4

struct game {
    // size of the board
    int width, height;
    // number of unique piece types
    int variety;
    // board state (with enough room for the larges board)
    char board[MAX_HEIGHT][MAX_WIDTH];
    // score
    uint16_t score;
};

// generic "point on the board" structure
struct point {
    int8_t row;
    int8_t column;
    // extra data about this point
    // (used for "activeness" of selection)
    int8_t meta;
};

char bggame_random_piece(struct game);
void bggame_board_init(struct game *game);
void bggame_move_cursor(struct game game,
                        uint8_t buttons_pushed,
                        struct point* cursor);
uint8_t bggame_are_neighbor_rowcols(int rc1, int rc2, int max);
uint8_t bggame_are_neighbors(struct game game,
                             struct point p1,
                             struct point p2);
void bggame_invalidate_selection(struct point* selection);
uint8_t bggame_selection_is_active(struct point selection);
void bggame_clear_selection(struct game *game, struct point* selection);
void bggame_set_selection(struct game *game,
                          struct point* selection,
                          struct point cursor);
int bggame_next_row(struct game game, int r);
int bggame_next_column(struct game game, int c);
int bggame_match(char a, char b, char c);
int bggame_mark_sets(struct game *game);
uint8_t bggame_remove_sets(struct game *game);
void bggame_swap_pieces(struct game *game, struct point a, struct point b);
uint8_t bggame_select(struct game *game,
                      uint8_t buttons_pushed,
                      struct point cursor,
                      struct point *selection);
void bggame_write_board(struct game game);
int bggame_first_space(char* row, int width);
void bggame_shift(char* row, int width, int start);
int bggame_fill_spaces_row(struct game game, char* row);
int bggame_fill_spaces(struct game *game);
void bggame_animate_space_fill(struct game *game);
void bggame_animate_clear_sets(struct game *game);
void bggame_clear_marks(struct game *game);
int8_t bggame_valid_move(struct game game, struct point a, struct point b);
int8_t bggame_valid_move_exists(struct game game);
void bggame_play(struct game *game);
void bggame_over(uint16_t score);
#endif
