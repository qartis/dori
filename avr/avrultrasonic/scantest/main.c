#include <ctype.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

int main(void) {
	uart_init(BAUD(38400));

    printf("system start\n");
    
    ICR1 = 20560;
    TCCR1A = (1 << COM1B1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    OCR1B = 1285;

    MCUCR |= (1 << ISC00);
    GICR |= (1 << INT0);

    TCCR0 = (1 << CS01);
    TIMSK = (1 << TOIE0);

    DDRD = (1 << PORTD3) | (1 << PORTD4);

    uint16_t filtered = 0;
    uint8_t giveup = 0;

    for(;;){
        if (pulse_len > 0 || giveup > 3) {
            cli();
            filtered = (filtered >> 2) + (pulse_len >> 2) + (pulse_len >> 1);
            pulse_len = 0;
            PORTD |= (1 << PORTD3);
            sei();
            _delay_us(10);
            PORTD &= ~(1 << PORTD3);

            uint16_t b = ((uint32_t)filtered * 11703) >> 16;
            uint16_t c = ((uint32_t)b * 6554) >> 16;
            printf("%u.%u cm\n", c, b%10);

            giveup = 0;
        }
        if (uart_haschar()) {
            int c = getchar();
            if (c == '='){
                OCR1B += 10;
            } else if (c == '-'){
                OCR1B -= 10;
            }
//            printf("%u\n", OCR1B);
        }
        giveup++;
        _delay_ms(50);
    }
}
