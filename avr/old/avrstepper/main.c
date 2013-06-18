#include <stdio.h>
#include <ctype.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include <avr/io.h>

#include "uart.h"
#include "motor.h"
//#include "mcp2515.h"

/*
void mcp2515_irq_callback(void)
{

}
*/


void adc_init(void)
{
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    DDRC |= (1 << PORTC5);
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

    PORTC |= (1 << PORTC5);
    val = adc_read(4);
    PORTC &= ~(1 << PORTC5);

    if (val < 9) {
        val = 0;
    } else if (val > 509) {
        val = 500;
    } else {
        val -= 9;
    }

    return val;
}


void main(void)
{
    uint8_t rc;
    uint8_t i;

    wdt_disable();

    uart_init(BAUD(38400));
    motor_init();
    adc_init();

/*
reinit:
    rc = mcp2515_init();
    if (rc)
        goto reinit;
        */
    printf("init: %x\n", MCUSR);

    MCUSR = 0;

    uint16_t val;
#if 0
    for (;;) {
        val = get_arm_angle();
        printf("val: %d (%d)\n", val, val/5);

        _delay_ms(300);
    }
#endif

    PORTB &= ~(1 << PORTB0);
    PORTD &= ~(1 << PORTD7);
    DDRB |= (1 << PORTB0);
    DDRD |= (1 << PORTD7);

    char ch;
    for (;;) {
        ch = uart_getchar();
        printf("%d\n", get_arm_angle());
        if (isdigit(ch)) {
            uint16_t goal = (ch - '0') * 50;
            if (goal < 10) goal = 10;
            if (get_arm_angle() > goal) {
                PORTD |= (1 << PORTD7);
                while (get_arm_angle() > goal) { _delay_ms(50); };
                PORTD &= ~(1 << PORTD7);
            } else {
                PORTB |= (1 << PORTB0);
                while (get_arm_angle() < goal) { _delay_ms(50); };
                PORTB &= ~(1 << PORTB0);
            }
            continue;
        }

        switch (ch) {
        case 'd':
            PORTB |= (1 << PORTB0);
            _delay_ms(50);
            PORTB &= ~(1 << PORTB0);
            break;
        case 'u':
            PORTD |= (1 << PORTD7);
            _delay_ms(50);
            PORTD &= ~(1 << PORTD7);
            break;
        case 'D':
            PORTB |= (1 << PORTB0);
            while (get_arm_angle() < 400) { _delay_ms(50); };
            PORTB &= ~(1 << PORTB0);
            break;
        case 'U':
            PORTD |= (1 << PORTD7);
            while (get_arm_angle() > 10) { _delay_ms(50); };
            PORTD &= ~(1 << PORTD7);
            break;
        case 'x':
            val = get_arm_angle();
            printf("val: %d (%d)\n", val, val/5);
            break;
        case 'a':
            motor_cw();
            break;
        case 's':
            motor_ccw();
            break;
        case 'A':
            for (i = 0; i < 100; i++) {
                motor_cw();
                _delay_ms(20);
            }
            break;
        case 'S':
            for (i = 0; i < 100; i++) {
                motor_ccw();
                _delay_ms(20);
            }
            break;
        default:
            motor_stop();
        }
    }
}
