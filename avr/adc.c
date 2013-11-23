#include <avr/io.h>
#include <stdint.h>

#include "adc.h"

int16_t map(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max)
{
      return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void adc_init(void)
{
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel)
{
    ADMUX = (1 << REFS0) | channel; /* AVCC */

    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) {};

    uint8_t low = ADCL;
    uint8_t high = ADCH;
    return ((uint16_t)(high<<8)) | low;
}
