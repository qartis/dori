#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "arm.h"

void adc_init(void)
{
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel)
{
    ADMUX = (1 << REFS0) | channel; /* internal vref */

    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) {};

    uint8_t low = ADCL;
    uint8_t high = ADCH;
    return ((uint16_t)(high<<8)) | low;
}

uint16_t get_arm_angle(void)
{
    uint16_t val;

    val = adc_read(4);

    if (val < 30) {
        val = 0;
    } else {
        val -= 30;
    }

    printf("angle %d\n", val);

    return val;
}

void set_arm_angle(uint8_t pos)
{

    DDRC |= (1 << PORTC1) | (1 << PORTC0);
    /* pos is 0..9 */
    uint16_t goal = pos * 61;
    uint16_t now = get_arm_angle();
    printf("goal: %d\n", goal);
    printf("now: %d\n", now);
    if ((now + 20) > goal && (now - 20) < goal) {
        //nothing
    } else if (now > goal) {
        PORTC |=  (1 << PORTC1);
        while (get_arm_angle() > goal) { _delay_ms(10); };
        PORTC &= ~(1 << PORTC1);
    } else {
        PORTC |=  (1 << PORTC0);
        while (get_arm_angle() < goal) { _delay_ms(10); };
        PORTC &= ~(1 << PORTC0);
    }
}
