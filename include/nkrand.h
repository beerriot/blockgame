#ifndef __NKRAND_H__
#define __NKRAND_H__

void boot_adc();
uint8_t adc_get_next_bit();
uint16_t random_seed_from_ADC();

#endif
