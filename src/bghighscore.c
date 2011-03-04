// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

// details about the game clock

#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "lcd.h"

#include "nkbuttons.h"
#include "nkeeprom.h"
#include "nklcd.h"
#include "nktimer.h"

#include "bghighscore.h"

bghighscore_t highscores[HIGH_SCORES];

void bghighscore_init() {
    if (!bghighscore_read()) {
        bghighscore_clear();
        bghighscore_write();
    }
}

uint8_t bghighscore_checksum() {
    uint8_t i, x = 0;
    unsigned char *hs = (unsigned char*)&highscores;
    for (i = 0; i < HIGH_SCORES*sizeof(bghighscore_t); i++)
        x ^= *(hs+i);
    return x;
}

uint8_t bghighscore_read() {
    uint8_t x, s, c;
    cli(); // disable interrupts
    nkeeprom_read_bytes((unsigned char*)&highscores,
                        0,
                        HIGH_SCORES*sizeof(bghighscore_t));
    nkeeprom_read_bytes(&x,
                        HIGH_SCORES*sizeof(bghighscore_t),
                        1);
    sei();
    for (s = 0; s < HIGH_SCORES; s++)
        for (c = 0; c < INITIALS; c++)
            if (highscores[s].initials[c] < 'a' ||
                highscores[s].initials[c] > 'z')
                return 0;
    return x == bghighscore_checksum();
}

void bghighscore_clear() {
    int8_t s, c;
    for (s = 0; s < HIGH_SCORES; s++) {
        for (c = 0; c < INITIALS; c++)
            highscores[s].initials[c] = 'a';
        highscores[s].score = 0;
    }
}

void bghighscore_write() {
    uint8_t x = bghighscore_checksum();
    cli(); //disable interrupts
    nkeeprom_write_bytes((unsigned char *)&highscores,
                         0,
                         HIGH_SCORES*sizeof(bghighscore_t));
    nkeeprom_write_bytes((unsigned char *)&x,
                         HIGH_SCORES*sizeof(bghighscore_t),
                         1);
    sei();
}

void bghighscore_display_line(int8_t rank, int8_t lcd_line) {
    int8_t c;
    lcd_goto_position(lcd_line, 6);
    for (c = 0; c < INITIALS; c++)
        lcd_write_data(highscores[rank].initials[c]);
    lcd_write_byte(' ');
    lcd_write_int16(highscores[rank].score);
}

void bghighscore_screen() {
    int8_t s;
    // game is over (no more moves)
    lcd_clear_and_home();
    nklcd_stop_blinking();
    lcd_goto_position(0, 5);
    lcd_write_string(PSTR("HIGH SCORES"));
    for (s = 0; s < HIGH_SCORES; s++) {
        bghighscore_display_line(s, 1+s);
    }

    nktimer_simple_delay(300);
}

void alter_highscore_initials(uint8_t buttons, int8_t rank, int8_t i) {
    if (buttons & B_UP) {
        if (highscores[rank].initials[i] < 'z')
            highscores[rank].initials[i]++;
        else
            highscores[rank].initials[i] = 'a';
    } else if (buttons & B_DOWN) {
        if (highscores[rank].initials[i] > 'a')
            highscores[rank].initials[i]--;
        else
            highscores[rank].initials[i] = 'z';
    }
}

uint8_t bghighscore_move_cursor(uint8_t buttons, int8_t *i) {
    if (buttons & B_LEFT) {
        if (*i > 0)
            *i -= 1;
        else
            *i = 2;
    } else if (buttons & (B_RIGHT | B_SELECT)) {
        if (*i < 2)
            *i += 1;
        else if (buttons & B_SELECT)
            return 1;
        else
            *i = 0;
    }
    return 0;
}

void bghighscore_new(int8_t rank, uint16_t score) {
    int8_t i;
    nkbuttons_t button_state;
    uint8_t pressed_buttons;
    nkbuttons_clear(&button_state);
    for (i = 0; i < INITIALS; i++)
        highscores[rank].initials[i] = 'a';
    highscores[rank].score = score;

    lcd_clear_and_home();
    lcd_goto_position(0, 2);
    lcd_write_string(PSTR("NEW HIGH SCORE"));
    bghighscore_display_line(rank, 2);
    lcd_goto_position(2, 6);
    nklcd_start_blinking();
    i = 0;

    while(1) {
        if (nktimer_animate()) {
            pressed_buttons = nkbuttons_read(&button_state);

            if(pressed_buttons) {
                nklcd_stop_blinking();
                alter_highscore_initials(pressed_buttons, rank, i);
                if(bghighscore_move_cursor(pressed_buttons, &i))
                    break;
                bghighscore_display_line(rank, 2);
                lcd_goto_position(2, 6+i);
                nklcd_start_blinking();
            }
        }
    }

    bghighscore_write();
}

void bghighscore_maybe(uint16_t score) {
    int8_t rank, shift, c;
    for (rank = 0; rank < HIGH_SCORES; rank++)
        if (score > highscores[rank].score) {
            for (shift = HIGH_SCORES-1; shift > rank; shift--) {
                for (c = 0; c < INITIALS; c++)
                    highscores[shift].initials[c] =
                        highscores[shift-1].initials[c];
                highscores[shift].score = highscores[shift-1].score;
            }
            bghighscore_new(rank, score);
            return;
        }
}
