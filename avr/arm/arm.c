#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "arm.h"

int16_t map(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max)
{
      return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

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

    //printf("angle %d\n", val);

    return val;
}

void set_arm_angle(uint8_t pos)
{
    DDRC |= (1 << PORTC1) | (1 << PORTC0);
    /* pos is 0..9 */
    uint16_t goal = map(pos, 0, 9, 530, 1000);

    uint16_t now = get_arm_angle();
    printf_P(PSTR("goal: %d"), goal);
    printf_P(PSTR("now: %d"), now);
    if ((now + 20) > goal && (now - 20) < goal) {
        puts_P(PSTR("done"));
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
