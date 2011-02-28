#ifndef __NKEEPROM_H__
#define __NKEEPROM_H__

char nkeeprom_read_byte(uint16_t address);
void nkeeprom_write_byte(char byte, uint16_t address);
void nkeeprom_read_bytes(unsigned char *dest, int offset, int count);
void nkeeprom_write_bytes(unsigned char *src, int offset, int count);

#endif
