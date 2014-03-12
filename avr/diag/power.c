#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

#include "adc.h"
#include "power.h"

#define SAMPLES 2048

uint16_t get_voltage(void)
{
    uint16_t i;
    uint32_t val;

    PORTC &= ~(1 << PORTC0);
    DDRC |= (1 << PORTC0);

    _delay_ms(10);

    val = 0;

    for (i = 0; i < SAMPLES; i++) {
        val += adc_read(6);
        _delay_us(100);
    }

    DDRC &= ~(1 << PORTC0);

    return val / SAMPLES;
}

uint16_t get_current(void)
{
    uint32_t val;
    uint16_t i;
    uint16_t raw;

    val = 0;

    for (i = 0; i < SAMPLES; i++) {
        val += adc_read(7);
        _delay_us(100);
    }

    raw = val / SAMPLES;

    /*
    1008 -> 0mA
    720  -> 162mA
    */

    if (raw > 1008) {
        raw = 1008;
    }

    raw = 1008 - raw;

    raw = (raw >> 1) + (raw >> 4); /* (raw * 162.0) / (1008 - 720) */

    return raw;
}
