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

void _delay_ms_long(uint16_t arg)
{
    while (arg--)
        _delay_ms(1);
}

void motor_drive(int16_t left_ms, int16_t right_ms)
{
    if (left_ms > 0) {
        PORTD |=  (1 << PORTD4);
    } else if (left_ms < 0) {
        left_ms = -left_ms;
        PORTD |=  (1 << PORTD5);
    }

    if (right_ms > 0) {
        PORTD |=  (1 << PORTD6);
    } else if (right_ms < 0) {
        right_ms = -right_ms;
        PORTD |=  (1 << PORTD7);
    }

    if (left_ms > right_ms) {
        _delay_ms_long(right_ms);
        PORTD &= ~((1 << PORTD6) | (1 << PORTD7));
        _delay_ms_long(left_ms - right_ms);
    } else {
        _delay_ms_long(left_ms);
        PORTD &= ~((1 << PORTD4) | (1 << PORTD5));
        _delay_ms_long(right_ms - left_ms);
    }

    PORTD &= ~((1 << PORTD4) | (1 << PORTD5) | (1 << PORTD6) | (1 << PORTD7));
}
