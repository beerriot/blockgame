// blockgame
// for NerdKits with ATmega168
// copyright 2011 Bryan Fink
// license: see LICENSE.txt

// using the ADC to produce random numbers

#include <inttypes.h>
#include <avr/pgmspace.h>

#include "nkrand.h"

void nkrand_init() {
    // set analog to digital converter
    // for external reference (5v), single ended input ADC0
    ADMUX = 0;
 
    // set analog to digital converter
    // to be enabled, with a clock prescale of 1/128
    // so that the ADC clock runs at 115.2kHz.
    ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
 
    // fire a conversion just to get the ADC warmed up
    ADCSRA |= (1<<ADSC);
}

void nkrand_close() {
    ADCSRA = 0;
}

uint8_t nkrand_next_bit() {
    // Lowest bit from ADC is the one most likely to change due to minute
    // variations in temperature as measured by LM37 and noise in power
    // supply.  Noise is generally a bad thing, but in our case,
    // the more - the better!  This function reads just the lowest bit
    // from ADC and discards the rest.
 
    // Wait until ADSC goes low (conversion completed).
    while (ADCSRA & (1<<ADSC)) { }
 
    // Read ADCL (AD low byte).  This one has the bit that we want.
    // (Mike mentions in tempsensor.c that ADCL has to be read first,
    // ATmega doc PDF page 259).
    uint16_t adc = ADCL;
    // read ADCH anyway to reset for the next conversion
    // do "something" with it to prevent compiler optimization
    adc ^= ADCH;
 
    // Start the next conversion.
    ADCSRA |= (1<<ADSC);
 
    // Return the lowest bit of ADCL.
    return adc & 1;
}
 
// Generate and return a random value.
uint16_t nkrand_seed() {
    uint16_t seed = 0;
    int8_t i;
 
    nkrand_init();

    // 'seed' is the value we are going to generate.
    // Starting with zeros in all 16 bits of the 16-bit unsigned integer,
    // we XOR the bits one by one with a highly volatile bit value from ADC,
    // and do it 100 times to mix things up really well.
    for (i = 0; i < 100; i++) {
        // XOR the seed with the random bit from ADC shifted
        seed ^= (nkrand_next_bit() << (i%16));
    }

    nkrand_close();

    return seed;
}
