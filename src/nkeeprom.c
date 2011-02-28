// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

// utilities for reading and writing the EEPROM

#include <avr/pgmspace.h>

#include "nkeeprom.h"

char nkeeprom_read_byte(uint16_t address) {
    // wait for completion of previous write)
    while (EECR & (1<<EEPE)) {}
    EEAR = address; //setup address
    EECR |= (1<<EERE); //start eeprom read
    return EEDR; // return data from register
}

void nkeeprom_write_byte(char byte, uint16_t address) {
    // wait for completion of previous write
    while (EECR & (1<<EEPE)) {}
    EEAR = address; //setup address
    EEDR = byte; //setup data
    EECR |= (1<<EEMPE); //enable writes
    EECR |= (1<<EEPE); //start write
}

void nkeeprom_read_bytes(unsigned char *dest, int offset, int count) {
    for (; count > 0; count--, dest++, offset++) 
        *dest = nkeeprom_read_byte(offset);
}

void nkeeprom_write_bytes(unsigned char *src, int offset, int count) {
    for(; count > 0; count--, src++, offset++)
        nkeeprom_write_byte(*src, offset);
}
