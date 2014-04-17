#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "arm.h"
#include "adc.h"

void arm_init(void)
{
    /* C2: set HI to measure arm */
    DDRC |= (1 << PORTC2);
    PORTC |= (1 << PORTC2);
}

uint16_t get_arm_angle(void)
{
    uint32_t val;
    uint8_t i;

#define NUM_SAMPLES 128

    val = 0;

    for (i = 0; i < NUM_SAMPLES; i++) {
        val += adc_read(4);
    }

    val /= NUM_SAMPLES;

    return val;
}

uint8_t set_arm_angle(uint16_t target_adc)
{
    uint16_t goal;
    uint16_t now;
    uint16_t retry;

    now = get_arm_angle();
    goal = target_adc;

    DDRD |= (1 << PORTD2) | (1 << PORTD7);

    printf_P(PSTR("goal: %d, "), goal);
    printf_P(PSTR("now: %d\n"), now);

    if (goal < 520) {
        goal = 520;
    } else if (goal > 1000) {
        goal = 1000;
    }

    if (now > goal) {
        PORTD |= (1 << PORTD2);

        /* 7650 * 1 = 7650 ms */
        retry = 7650;
        uint16_t angle;
        while ( (angle = get_arm_angle()) > (goal - 1) && --retry) {
            _delay_ms(1);
        }

        printf("retry: %d, angle: %d\n", retry, angle);

        PORTD &= ~(1 << PORTD2);

        return (retry == 0);
    } else {
        PORTD |= (1 << PORTD7);

        /* 7650 * 1 = 7650 ms */
        retry = 7650;
        uint16_t angle;
        while ( (angle = get_arm_angle()) < (goal + 1) && --retry) {
            _delay_ms(1);
        }

        printf("retry: %d, angle: %d\n", retry, angle);

        PORTD &= ~(1 << PORTD7);

        return (retry == 0);
    }
}
