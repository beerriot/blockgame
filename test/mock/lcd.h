/* lcd.h: Mock definitions for testing */
#ifndef __LCD_H
#define __LCD_H

void lcd_goto_position(uint8_t row, uint8_t col);
void lcd_write_data(char c);
void lcd_clear_and_home();
void lcd_write_string(const char *x);
void lcd_write_int16(int16_t in);
void lcd_write_byte(char c);
void lcd_init();
void lcd_set_type_command();
void lcd_home();

#endif
