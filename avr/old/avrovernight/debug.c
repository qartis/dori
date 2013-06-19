#include <stdio.h>
#include <stdarg.h>
#include <avr/io.h>
#include <util/delay.h>

#include "debug.h"

void debug_init(void)
{
    DDRB = (1 << PORTB0);
    PORTB = (1 << PORTB0);
}

void debugf(const char *fmt, ...)
{
    va_list ap;
    char buf[128];

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    debug(buf);
}

void debug(const char *str)
{
    uint8_t i;

    while (*str) {
        PORTB &= ~(1 << PORTB0);
        _delay_us(100);
        for (i = 0; i < 8; i++){
            if (*str & (1 << i)) {
                PORTB |= (1 << PORTB0);
            } else {
                PORTB &= ~(1 << PORTB0);
            }
            _delay_us(100);
        }
        PORTB |= (1 << PORTB0);
        _delay_us(200);
        str++;
    }
}
