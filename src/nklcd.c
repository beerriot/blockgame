// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

// utilities for manipulating the LCD

#include <inttypes.h>
#include "lcd.h" //add nerdkits-provided library

#include "nklcd.h"

// get the LCD setup at boot
void boot_lcd() {
    lcd_init();
    lcd_home();
}

void start_blinking() {
    lcd_set_type_command();
    lcd_write_byte(0x0F);
}

void stop_blinking() {
    lcd_set_type_command();
    lcd_write_byte(0x0C);
}
