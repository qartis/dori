#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "motor.h"

//D4..7

void motor_init(void)
{
    DDRD |= (1 << PORTD4) | (1 << PORTD5)
            | (1 << PORTD6) | (1 << PORTD7);

    PORTD = 0;
}

void motor_left(int16_t ms)
{
    if (ms > 0)
        PORTD |=  (1 << PORTD4);
    else
        PORTD |=  (1 << PORTD5);

    while (--ms)
        _delay_ms(1);

    PORTD &= ~((1 << PORTD4) | (1 << PORTD5));
}

void motor_right(int16_t ms)
{
    if (ms > 0)
        PORTD |=  (1 << PORTD6);
    else
        PORTD |=  (1 << PORTD7);

    while (--ms)
        _delay_ms(1);

    PORTD &= ~((1 << PORTD6) | (1 << PORTD7));
}
