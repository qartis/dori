#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "uart.h"

#define DELAY do { \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
    __asm__("nop"); \
} while (0);

void main(void)
{
    uint8_t state[4];
    uint8_t gnd[4];

    uart_init(4);

    for (;;) {
        while ((PINC & (1 << PORTC5))) { };
        while (!(PINC & (1 << PORTC5))) { };
        _delay_us(100);
        state[0] = PINC;

        while ((PINC & (1 << PORTC4))) { };
        while (!(PINC & (1 << PORTC4))) { };
        _delay_us(100);
        state[1] = PINC;

        while ((PINC & (1 << PORTC3))) { };
        while (!(PINC & (1 << PORTC3))) { };
        _delay_us(100);
        state[2] = PINC;

        while ((PINC & (1 << PORTC2))) { };
        while (!(PINC & (1 << PORTC2))) { };
        _delay_us(100);
        state[3] = PINC;

        printf("bits:  %c \n", state[3] & (1 << PORTC1) ? ' ' : '_');
        printf("      %c",     state[3] & (1 << PORTC0) ? ' ' : '|');
        printf("%c",           state[2] & (1 << PORTC0) ? ' ' : '_');
        printf("%c\n",         state[2] & (1 << PORTC1) ? ' ' : '|');
        printf("      %c",     state[1] & (1 << PORTC0) ? ' ' : '|');
        printf("%c",           state[0] & (1 << PORTC1) ? ' ' : '_');
        printf("%c\n\n",       state[1] & (1 << PORTC1) ? ' ' : '|');
        _delay_ms(500);
    }
}
