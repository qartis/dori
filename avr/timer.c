#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "timer.h"
#include "uart.h"

volatile uint8_t timeout_counter;

void timer_start(void)
{
    /* 8e6 / 256 is a whole integer (31250) so we pick
       that as our TOP value to give us a 1sec timer */
    TIMSK1 = (1<<OCIE1A);
    OCR1A = F_CPU / 256;
    TCNT1 = 0;
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1<<CS12);
    timeout_counter = 0;
}

void timer_disable(void)
{
    TCCR1B = 0;
    TIMSK1 = 0;
}

ISR(TIMER1_COMPA_vect)
{
    timeout_counter++;
}

int8_t read_timeout(uint8_t *buf, size_t count, uint8_t sec)
{
    size_t readbytes = 0;
    int ch;

    while (readbytes < count) {
        ch = getc_timeout(sec);
        if (ch == EOF){
            return -1;
        }
        *buf++ = ch;
        readbytes++;
    }
    return readbytes;
}

uint8_t getline_timeout(char *s, uint8_t n, uint8_t sec)
{
    int ch = EOF;
    uint8_t i = 0;

    timer_start();
restart:

    while (i++ < n && (ch = getc_timeout(sec)) != EOF) {
        if (ch == '\r'){
            ch = '\n';
        }
        *s++ = ch;
        if (ch == '\n'){
            break;
        }
    }

    /* if we just read an empty line, try again */
    if (i == 1 && s[-1] == '\n'){
        s -= 1;
        goto restart;
    }

    timer_disable();

    if (ch == EOF) {
        return 0;
    }

    *s = '\0';

    return i;
}
