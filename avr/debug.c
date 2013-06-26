#include <stdio.h>
#include <stdarg.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "debug.h"

void debug_init(void)
{
    /* tx */
    DDRC |= (1 << PORTC0);
    PORTC |= (1 << PORTC0);

    /* rx */
    DDRC &= ~(1 << PORTC1);

    PCMSK1 = (1 << PCINT9);
    PCICR |= (1 << PCIE1);
}

volatile uint8_t debug_buf[64];
volatile uint8_t debug_buf_len;
volatile uint8_t debug_has_line;

void debug_reset(void)
{
    debug_buf_len = 0;
    debug_has_line = 0;
}

ISR(PCINT1_vect)
{
    uint8_t i;
    uint8_t c = 0;

    if (PINC & (1 << PINC1))
        return;

    if (debug_has_line)
        return;

    _delay_us(150);
    for (i = 0; i < 8; i++){
        c >>= 1;
        if (PINC & (1 << PINC1))
            c |= (1 << 7);

        _delay_us(100);
    }

    debug_putchar(c);

    if (c == '\r') {
        debug_putchar('\n');
        debug_has_line = 1;
        return;
    }

    debug_buf[debug_buf_len] = c;
    debug_buf[debug_buf_len + 1] = '\0';

    if (debug_buf_len < sizeof(debug_buf) - 1)
        debug_buf_len++;
}

void debug(const char *fmt, ...)
{
    va_list ap;
    char buf[128];

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    char *str = buf;

    while (*str) {
crnl:

        debug_putchar(*str);
        
        if (*str == '\n') {
            *str = '\r';
            goto crnl;
        }

        str++;
    }
}


void debug_putchar(uint8_t c)
{
    uint8_t i;

    PORTC &= ~(1 << PORTC0);
    _delay_us(100);
    for (i = 0; i < 8; i++){
        if (c & (1 << i)) {
            PORTC |= (1 << PORTC0);
        } else {
            PORTC &= ~(1 << PORTC0);
        }
        _delay_us(100);
    }
    PORTC |= (1 << PORTC0);
    _delay_us(150);
}
