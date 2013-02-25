#include <ctype.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <stdio.h>

#include "uart.h"

volatile uint16_t pulse_len;
uint8_t highbyte;

ISR(INT0_vect)
{
    static uint16_t pulse_start;

    if (!(PIND & (1 << PORTD2))) {
        pulse_len = (highbyte << 8) | TCNT0;
        if (pulse_start > pulse_len) {
            pulse_len = (0xffff - pulse_start) + pulse_len;
        } else {
            pulse_len -= pulse_start;
        }
    } else {
        pulse_start = (highbyte << 8) | TCNT0;
    }
}

ISR(TIMER0_OVF_vect)
{
    highbyte++;
}

int main(void)
{
    wdt_disable();
    uart_init(BAUD(38400));
    _delay_ms(500);
    printf("system start\n");

    MCUCR |= (1 << ISC00);
    GICR |= (1 << INT0);

    TCCR0 = (1 << CS01); /* CLKio / 8 */
    TIMSK = (1 << TOIE0);

    DDRD = (1 << PORTD3);

    uint16_t filtered = 0;
    uint8_t giveup = 0;

    for(;;){
        if (pulse_len > 0 || giveup > 3) {
            cli();
            filtered = (filtered >> 2) + (pulse_len >> 2) + (pulse_len >> 1);
            pulse_len = 0;
            _delay_ms(60);
            PORTD |= (1 << PORTD3);
            sei();
            _delay_us(10);
            PORTD &= ~(1 << PORTD3);

            uint16_t b = ((uint32_t)filtered * 11703) >> 16;
            uint16_t c = ((uint32_t)b * 6554) >> 16;
            printf("%u.%u cm\n", c, b%10);

            giveup = 0;
        }
        giveup++;
        _delay_ms(60);
    }
}
