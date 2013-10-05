#include <stdio.h>
#include <util/atomic.h>
#include <stdarg.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "debug.h"
#include "irq.h"

#define DEBUG_BUF_SIZE 64
//#define DEBUG_FAST

volatile uint8_t debug_buf[DEBUG_BUF_SIZE];
volatile uint8_t debug_buf_in;
volatile uint8_t debug_buf_out;

static FILE mystdout = FDEV_SETUP_STREAM(
    (int (*)(char,FILE*))debug_putchar, 
    (int (*)(FILE*))debug_getchar,
    _FDEV_SETUP_RW);

void debug_init(void)
{
    /* tx */
    DDRC |= (1 << PORTC0);
    PORTC |= (1 << PORTC0);

    /* rx */
    DDRC &= ~(1 << PORTC1);
    PORTC |= (1 << PORTC1);

    PCMSK1 = (1 << PCINT9);
    PCICR |= (1 << PCIE1);
    
    stdout = &mystdout;
    stdin = &mystdout;
}

ISR(PCINT1_vect)
{
    uint8_t i;
    uint8_t c = 0;

    if (PINC & (1 << PINC1))
        return;

#ifdef DEBUG_FAST
    /* 38400 baud */
    _delay_us(32);
    for (i = 0; i < 8; i++){
        c >>= 1;
        if (PINC & (1 << PINC1))
            c |= (1 << 7);

        _delay_us(24);
    }
#else
    /* 9600 baud */
    _delay_us(150);
    for (i = 0; i < 8; i++){
        c >>= 1;
        if (PINC & (1 << PINC1))
            c |= (1 << 7);

        _delay_us(100);
    }
#endif

    if (c == '\r')
        c = '\n';
        
    debug_buf[debug_buf_in] = c;
    debug_buf_in = (debug_buf_in + 1) % DEBUG_BUF_SIZE;

    debug_putchar(c);

    if (c == '\n')
        irq_signal |= IRQ_DEBUG;
}

uint8_t debug_getchar(void)
{
    uint8_t c;

    while (debug_buf_in == debug_buf_out) {};
    
    c = debug_buf[debug_buf_out];
    debug_buf_out = (debug_buf_out + 1) % DEBUG_BUF_SIZE;

    return c;
}

void debug_putchar(char c)
{
    uint8_t i;

again:

ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

    PORTC &= ~(1 << PORTC0);


#ifdef DEBUG_FAST
    _delay_us(22);
    for (i = 0; i < 8; i++){
        if (c & (1 << i))
            PORTC |= (1 << PORTC0);
        else
            PORTC &= ~(1 << PORTC0);

        _delay_us(22);
    }

    PORTC |= (1 << PORTC0);
    _delay_us(22);
#else
    _delay_us(100);
    for (i = 0; i < 8; i++){
        if (c & (1 << i))
            PORTC |= (1 << PORTC0);
        else
            PORTC &= ~(1 << PORTC0);

        _delay_us(100);
    }

    PORTC |= (1 << PORTC0);
    _delay_us(100);
#endif
}
    
    if (c == '\n') {
        c = '\r';
        goto again;
    }
}

void debug_flush(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        debug_buf_in = debug_buf_out;
    }
}
