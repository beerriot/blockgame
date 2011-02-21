#ifndef __EEPROM_H__
#define __EEPROM_H__

char read_eeprom_byte(uint16_t address);
void write_eeprom_byte(char byte, uint16_t address);
void read_eeprom_bytes(unsigned char* dest, int offset, int count);
void write_eeprom_bytes(unsigned char* src, int offset, int count);

#endif
