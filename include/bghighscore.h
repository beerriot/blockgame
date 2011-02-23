#ifndef __BGHIGHSCORE_H__
#define __BGHIGHSCORE_H__

// number of high scores we keep around
#define HIGH_SCORES 3
#define INITIALS 3

// the high score table
struct bghighscore {
    // initials of the player that made the score
    char initials[INITIALS];
    // the player's score
    uint16_t score;
};

void bghighscore_init();
uint8_t bghighscore_checksum();
uint8_t bghighscore_read();
void bghighscore_clear();
void bghighscore_write();
void bghighscore_display_line(int rank, int lcd_line);
void bghighscore_screen();
void bghighscore_alter_initials(uint8_t buttons, int rank, int i);
int bghighscore_move_cursor(uint8_t buttons, int* i);
void bghighscore_new(int rank, uint16_t score);
void bghighscore_maybe(uint16_t score);

#endif
