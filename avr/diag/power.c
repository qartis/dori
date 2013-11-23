#include <avr/io.h>
#include <util/delay.h>

#include "adc.h"
#include "power.h"

void power_init(void)
{
}

uint16_t get_voltage(void)
{
    uint16_t val;

    PORTC &= ~(1 << PORTC0);
    DDRC |= (1 << PORTC0);
    PORTC &= ~(1 << PORTC0);

    _delay_ms(100);

    val = adc_read(6);
    _delay_ms(100);

    DDRC &= ~(1 << PORTC0);

    return val;
}

uint16_t get_current(void)
{
    uint16_t val;

    val = adc_read(7);

    return val;
}
