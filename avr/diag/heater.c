#include <avr/io.h>

#include "heater.h"

void heater_init(void)
{
    DDRD |= (1 << PORTD4) | (1 << PORTD5)
            | (1 << PORTD6) | (1 << PORTD7);

    PORTD = 0;
}


void heater_on(uint8_t channel)
{
    PORTD |= (1 << (PORTD4 + channel));
}

void heater_off(uint8_t channel)
{
    PORTD &= ~(1 << (PORTD4 + channel));
}
