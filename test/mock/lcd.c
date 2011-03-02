/* lcd.c: Mock definitions for testing */

#include <inttypes.h>
#include <stdio.h>

void lcd_goto_position(uint8_t row, uint8_t col) {
    printf("%s:%d\n", __FILE__, __LINE__);
}
void lcd_write_data(char c) {
    printf("%s:%d\n", __FILE__, __LINE__);
}
void lcd_clear_and_home() {
    printf("%s:%d\n", __FILE__, __LINE__);
}
void lcd_write_string(const char *x) {
    printf("%s:%d\n", __FILE__, __LINE__);
}
void lcd_write_int16(int16_t in) {
    printf("%s:%d\n", __FILE__, __LINE__);
}
void lcd_write_byte(char c) {
    printf("%s:%d\n", __FILE__, __LINE__);
}
void lcd_init() {
    printf("%s:%d\n", __FILE__, __LINE__);
}
void lcd_set_type_command() {
    printf("%s:%d\n", __FILE__, __LINE__);
}
void lcd_home() {
    printf("%s:%d\n", __FILE__, __LINE__);
}
