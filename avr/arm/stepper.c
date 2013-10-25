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

void set_stepper_angle(int16_t goal)
{
    int32_t i;
    if (goal > 9) {
        for (i = 0; i < goal; i++) {
            stepper_cw();
            _delay_us(200);
            _delay_ms(1);
        }
    } else if (goal < 0) {
        for (i = goal; i < 0; i++) {
            stepper_ccw();
            _delay_us(200);
            _delay_ms(1);
        }
    }
}

uint16_t get_stepper_angle(void)
{
    return 5; // random number for now
}
