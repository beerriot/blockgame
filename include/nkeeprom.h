#ifndef __NKEEPROM_H__
#define __NKEEPROM_H__

char nkeeprom_read_byte(uint16_t address);
void nkeeprom_write_byte(char byte, uint16_t address);
void nkeeprom_read_bytes(unsigned char *dest, uint16_t offset, int16_t count);
void nkeeprom_write_bytes(unsigned char *src, uint16_t offset, int16_t count);

#endif
