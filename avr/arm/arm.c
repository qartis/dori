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
    uint16_t val;

    val = adc_read(4);

    return val;
}

uint8_t set_arm_percent(uint8_t pos)
{
    uint16_t goal;
    uint16_t now;
    uint8_t retry;

    /* pos is 0..9 */
    goal = map(pos, 0, 9, 530, 1000);
    now = get_arm_angle();

    DDRD |= (1 << PORTD2) | (1 << PORTD7);

    printf_P(PSTR("goal: %d"), goal);
    printf_P(PSTR("now: %d"), now);

    if ((now + 20) > goal && (now - 20) < goal) {
        puts_P(PSTR("done"));

        return 0;
    } else if (now > goal) {
        PORTD |= (1 << PORTD2);

        /* 255 * 30 = 7650 ms */
        retry = 255;
        while (get_arm_angle() > goal && --retry) {
            _delay_ms(30);
        }

        PORTD &= ~(1 << PORTD2);

        return (retry == 0);
    } else {
        PORTD |= (1 << PORTD7);

        /* 255 * 30 = 7650 ms */
        retry = 255;
        while (get_arm_angle() < goal && --retry) {
            _delay_ms(30);
        }

        PORTD &= ~(1 << PORTD7);

        return (retry == 0);
    }
}
