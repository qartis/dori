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

    _delay_ms(100);

    val = adc_read(6);

    DDRC &= ~(1 << PORTC0);

    return val;
}

uint16_t get_current(void)
{
    uint16_t val;

    val = adc_read(7);

    /* 512 = 0mA, <512 means negative current. we don't
       care what direction so we take the abs() */
    if (val < 512)
        val = 512 - val;
    else
        val -= 512;

    return val;
}
