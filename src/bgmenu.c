// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

// workings of the startup menu

#include <inttypes.h>
#include <avr/pgmspace.h>

#include "lcd.h"

#include "nkbuttons.h"
#include "nklcd.h"
#include "nktimer.h"

#include "bggame.h"
#include "bgmenu.h"

// line numbers of prompts
#define P_WIDTH   0
#define P_HEIGHT  1
#define P_VARIETY 2
#define P_START   3
#define P_LAST    3

void bgmenu_forb(int8_t row, uint8_t focus) {
    lcd_goto_position(row, 10);
    lcd_write_data(focus ? 0x7e : ' ');
    lcd_goto_position(row, 16);
    lcd_write_data(focus ? 0x7f : ' ');
}
void bgmenu_focus(int8_t row) {
    bgmenu_forb(row, 1);
}
void bgmenu_blur(int8_t row) {
    bgmenu_forb(row, 0);
}

void bgmenu_write_prompt(int8_t row, int8_t val) {
    lcd_goto_position(row, 12);
    if (val < 10)
        lcd_write_data(' ');
    lcd_write_int16(val);
}

int8_t bgmenu_previous_prompt(int8_t current) {
    return bgmenu_refocus(current, (current == 0) ? P_LAST : current-1);
}

int8_t bgmenu_next_prompt(int8_t current) {
    return bgmenu_refocus(current, (current == P_LAST) ? 0 : current+1);
}

int8_t bgmenu_refocus(int8_t blur, int8_t focus) {
    bgmenu_blur(blur);
    bgmenu_focus(focus);
    return focus;
}

int8_t bgmenu_field_and_limits(int8_t prompt, game_t *game,
                               int8_t **field, int8_t *min, int8_t *max) {
    switch (prompt) {
    case P_WIDTH:
        *field = &game->width;
        *min = 10;
        *max = MAX_WIDTH;
        return 1;
    case P_HEIGHT:
        *field = &game->height;
        *min = 3;
        *max = MAX_HEIGHT;
        return 1;
    case P_VARIETY:
        *field = &game->variety;
        *min = 5;
        *max = 26;
        return 1;
    default:
        return 0; // start doesn't increase
    }
}

void bgmenu_increase_prompt(int8_t prompt, game_t *game) {
    int8_t *field, min, max;
    if (bgmenu_field_and_limits(prompt, game, &field, &min, &max)) {
        if (*field < max)
            *field = *field+1;
        bgmenu_write_prompt(prompt, *field);
    }
}

void bgmenu_decrease_prompt(int8_t prompt, game_t *game) {
    int8_t *field, min, max;
    if (bgmenu_field_and_limits(prompt, game, &field, &min, &max)) {
        if (*field > min)
            *field = *field-1;
        bgmenu_write_prompt(prompt, *field);
    }
}

uint8_t bgmenu_display(game_t *game) {
    uint8_t ready = 0;
    int8_t prompt = 0;
    int16_t inactivity = 0;
    uint8_t pressed_buttons;
    nkbuttons_t button_state;
    nkbuttons_clear(&button_state);
    nklcd_stop_blinking();
    lcd_clear_and_home();

    lcd_goto_position(P_WIDTH, 3);
    lcd_write_string(PSTR("width:"));
    bgmenu_write_prompt(P_WIDTH, game->width);
    bgmenu_focus(P_WIDTH);

    lcd_goto_position(P_HEIGHT, 2);
    lcd_write_string(PSTR("height:"));
    bgmenu_write_prompt(P_HEIGHT, game->height);

    lcd_goto_position(P_VARIETY, 1);
    lcd_write_string(PSTR("variety:"));
    bgmenu_write_prompt(P_VARIETY, game->variety);

    lcd_goto_position(P_START, 11);
    lcd_write_string(PSTR("start"));

    while(!ready) {
        if (nktimer_animate()) {
            pressed_buttons = nkbuttons_read(&button_state);
            if(pressed_buttons) {
                inactivity = 0;
                if (pressed_buttons & B_SELECT && prompt == P_START) {
                    ready = 1;
                } else if (pressed_buttons & B_UP) {
                    prompt = bgmenu_previous_prompt(prompt);
                } else if (pressed_buttons & (B_DOWN | B_SELECT)) {
                    prompt = bgmenu_next_prompt(prompt);
                } else if (pressed_buttons & B_LEFT) {
                    bgmenu_decrease_prompt(prompt, game);
                } else {
                    bgmenu_increase_prompt(prompt, game);
                }
            } else if (++inactivity > 600) {
                return 0; // leave menu for "screen saver" after 10sec
            }
        }
    }
    return 1;
}

