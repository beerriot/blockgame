// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

// utilities for manipulating the LCD

#include <inttypes.h>
#include "lcd.h" //add nerdkits-provided library

#include "nklcd.h"

// get the LCD setup at boot
void nklcd_init() {
    lcd_init();
    lcd_home();
}

void nklcd_start_blinking() {
    lcd_set_type_command();
    lcd_write_byte(DISPLAY_CMD|DISPLAY_ON|DISPLAY_CURSOR|DISPLAY_BLINK);
}

void nklcd_stop_blinking() {
    lcd_set_type_command();
    lcd_write_byte(DISPLAY_CMD|DISPLAY_ON);
}

void nklcd_off() {
    lcd_set_type_command();
    lcd_write_byte(DISPLAY_CMD);
}

void nklcd_on() {
    lcd_set_type_command();
    lcd_write_byte(DISPLAY_CMD|DISPLAY_ON);
}
