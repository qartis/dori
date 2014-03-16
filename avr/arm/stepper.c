#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "stepper.h"

#define DELAY 1

int32_t stepper_state;

void stepper_init(void)
{
    STEPPER_DDR |= (1 << PORTD4) | (1 << PORTD5);
    DDRC |= (1 << STEPPER_SLP);

    stepper_sleep();

    stepper_set_state(-1);
}

void stepper_set_state(int32_t new_state)
{
    stepper_state = new_state;
}

int32_t stepper_get_state(void)
{
    return stepper_state;
}

void stepper_ccw(void)
{
    PORTD |= (1 << PORTD4);

    PORTD &= ~(1 << PORTD5);
    PORTD |= (1 << PORTD5);
    _delay_us(500);
}

void stepper_cw(void)
{
    PORTD &= ~(1 << PORTD4);

    PORTD &= ~(1 << PORTD5);
    PORTD |= (1 << PORTD5);
    _delay_us(500);
}

void stepper_sleep(void)
{
    PORTC &= ~(1 << STEPPER_SLP);
}

void stepper_wake(void)
{
    PORTC |=  (1 << STEPPER_SLP);
    _delay_ms(1);
}

/*
void stepper_set_state_buf(const uint8_t buf[static 4])
{
    stepper_state =
        (int32_t)((uint32_t)buf[0] << 24) |
        (int32_t)((uint32_t)buf[1] << 16) |
        (int32_t)((uint32_t)buf[2] << 8)  |
        (int32_t)((uint32_t)buf[3] << 0);
}
*/

uint8_t stepper_set_angle(int32_t goal)
{
    if (stepper_state == -1) {
        return 1;
    }

    if (goal > 158480) {
        return 2;
    }

    stepper_wake();

    if (stepper_state < goal) {
        while (stepper_state < goal) {
            stepper_cw();
            stepper_state++;
        }
    } else {
        while (stepper_state > goal) {
            stepper_ccw();
            stepper_state--;
        }
    }

    stepper_sleep();

    return 0;
}
