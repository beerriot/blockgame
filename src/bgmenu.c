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

void bgmenu_forb(int row, uint8_t focus) {
    lcd_goto_position(row, 10);
    lcd_write_data(focus ? 0x7e : ' ');
    lcd_goto_position(row, 16);
    lcd_write_data(focus ? 0x7f : ' ');
}
void bgmenu_focus(int row) {
    bgmenu_forb(row, 1);
}
void bgmenu_blur(int row) {
    bgmenu_forb(row, 0);
}

void bgmenu_write_prompt(int row, int val) {
    lcd_goto_position(row, 12);
    if (val < 10)
        lcd_write_data(' ');
    lcd_write_int16(val);
}

uint8_t bgmenu_display(struct game *game) {
    int ready = 0, prompt = 0, inactivity = 0;
    uint8_t pressed_buttons;
    int *field, min, max;
    struct nkbuttons button_state;
    nkbuttons_clear(&button_state);
    nklcd_stop_blinking();
    lcd_clear_and_home();

    lcd_goto_position(0, 3);
    lcd_write_string(PSTR("width:"));
    bgmenu_write_prompt(0, game->width);
    bgmenu_focus(0);

    lcd_goto_position(1, 2);
    lcd_write_string(PSTR("height:"));
    bgmenu_write_prompt(1, game->height);

    lcd_goto_position(2, 1);
    lcd_write_string(PSTR("variety:"));
    bgmenu_write_prompt(2, game->variety);

    lcd_goto_position(3, 11);
    lcd_write_string(PSTR("start"));

    while(!ready) {
        if (nktimer_animate()) {
            pressed_buttons = nkbuttons_read(&button_state);
            if(pressed_buttons) {
                inactivity = 0;
                if (pressed_buttons & B_SELECT && prompt == 3) {
                    ready = 1;
                } else if (pressed_buttons & (B_UP | B_DOWN | B_SELECT)) {
                    bgmenu_blur(prompt);
                    if (pressed_buttons & B_UP) {
                        prompt--;
                        if (prompt < 0) prompt = 3;
                    } else {
                        prompt++;
                        if (prompt > 3) prompt = 0;
                    }
                    bgmenu_focus(prompt);
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
                    bgmenu_write_prompt(prompt, *field);
                }
            } else if (++inactivity > 600) {
                return 0; // leave menu for "screen saver" after 10sec
            }
        }
    }
    return 1;
}

