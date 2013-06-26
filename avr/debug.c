#include <stdio.h>
#include <stdarg.h>
#include <avr/io.h>
#include <util/delay.h>

void debug_init(void)
{
    DDRC |= (1 << PORTC0);
    PORTC |= (1 << PORTC0);
}

void debug(const char *fmt, ...)
{
    uint8_t i;
    va_list ap;
    char buf[128];

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    char *str = buf;

    while (*str) {
crnl:

        PORTC &= ~(1 << PORTC0);
        _delay_us(100);
        for (i = 0; i < 8; i++){
            if (*str & (1 << i)) {
                PORTC |= (1 << PORTC0);
            } else {
                PORTC &= ~(1 << PORTC0);
            }
            _delay_us(100);
        }
        PORTC |= (1 << PORTC0);
        _delay_us(200);
        
        if (*str == '\n') {
            *str = '\r';
            goto crnl;
        }

        str++;
    }
}
