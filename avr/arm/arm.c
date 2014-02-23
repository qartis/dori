#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "arm.h"
#include "adc.h"

uint16_t get_arm_angle(void)
{
    uint16_t val;

    val = adc_read(4);

    return val;
}

void set_arm_percent(uint8_t pos)
{
    DDRD |= (1 << PORTD2) | (1 << PORTD7);

    /* pos is 0..9 */
    uint16_t goal = map(pos, 0, 9, 530, 1000);

    uint16_t now = get_arm_angle();
    printf_P(PSTR("goal: %d"), goal);
    printf_P(PSTR("now: %d"), now);
    if ((now + 20) > goal && (now - 20) < goal) {
        puts_P(PSTR("done"));
        //nothing
    } else if (now > goal) {
        printf("d2\n");
        PORTD |=  (1 << PORTD2);
        while (get_arm_angle() > goal) { _delay_ms(10); };
        PORTD &= ~(1 << PORTD2);
    } else {
        printf("d7\n");
        PORTD |=  (1 << PORTD7);
        while (get_arm_angle() < goal) { _delay_ms(10); };
        PORTD &= ~(1 << PORTD7);
    }
}
