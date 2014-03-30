#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "stepper.h"
#include "errno.h"

#define DELAY 1
#define TOTAL_STEPS 36000
#define TOTAL_TICKS 144000
#define TICKS_PER_STEP (TOTAL_TICKS / TOTAL_STEPS)

int32_t stepper_state;
uint16_t stepper_stepsize;

void stepper_init(void)
{
    STEPPER_DDR |= (1 << PORTD4) | (1 << PORTD5);
    DDRC |= (1 << STEPPER_SLP);

    stepper_sleep();

    stepper_set_state(-1);
    stepper_set_stepsize(1);
}

uint16_t stepper_get_stepsize(void)
{
    return stepper_stepsize;
}

uint8_t stepper_set_stepsize(uint16_t stepsize)
{
    if (stepsize > (TOTAL_STEPS / 4))
        return ERR_ARG;

    stepper_stepsize = stepsize;

    return 0;
}

int32_t stepper_get_state(void)
{
    return stepper_state;
}

uint8_t stepper_set_state(int32_t new_state)
{
    if (new_state < -1 || new_state > TOTAL_STEPS)
        return ERR_ARG;

    stepper_state = new_state;

    return 0;
}

void stepper_ccw(void)
{
    uint8_t i;
    uint8_t j;

    if (stepper_state == -1)
        return;

    PORTD |= (1 << PORTD4);

    for (i = 0; i < stepper_stepsize; i++) {
        for (j = 0; j < TICKS_PER_STEP; j++) {
            PORTD &= ~(1 << PORTD5);
            PORTD |= (1 << PORTD5);
            _delay_us(500);
        }
    }

    stepper_state -= stepper_stepsize;
}

void stepper_cw(void)
{
    uint8_t i;
    uint8_t j;

    if (stepper_state == -1)
        return;

    PORTD &= ~(1 << PORTD4);

    for (i = 0; i < stepper_stepsize; i++) {
        for (j = 0; j < TICKS_PER_STEP; j++) {
            PORTD &= ~(1 << PORTD5);
            PORTD |= (1 << PORTD5);
            _delay_us(500);
        }
    }

    stepper_state += stepper_stepsize;
}

void stepper_sleep(void)
{
    PORTC &= ~(1 << STEPPER_SLP);
}

void stepper_wake(void)
{
    PORTC |=  (1 << STEPPER_SLP);
    _delay_ms(5);
}

uint8_t stepper_set_angle(uint16_t goal)
{
    if (stepper_state == -1)
        return ERR_STEPPER_INIT;

    if (goal > 36000)
        return ERR_ARG;

    stepper_wake();

    while (stepper_state < goal)
        stepper_cw();

    while (stepper_state > goal)
        stepper_ccw();

    stepper_sleep();

    return 0;
}
