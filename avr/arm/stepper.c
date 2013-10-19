#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "stepper.h"

#define DELAY 1

void stepper_init(void)
{
    STEPPER_DDR |= (1 << PORTD4) | (1 << PORTD5);
}

void stepper_ccw(void)
{
    PORTD |= (1 << PORTD4);

    PORTD &= ~(1 << PORTD5);
    PORTD |= (1 << PORTD5);
    _delay_ms(1);
}

void stepper_cw(void)
{
    PORTD &= ~(1 << PORTD4);

    PORTD &= ~(1 << PORTD5);
    PORTD |= (1 << PORTD5);
    _delay_ms(1);
}

void stepper_stop(void)
{
}
