#include <stdio.h>
#include <stdarg.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "debug.h"

#define DEBUG_BUF_SIZE 64

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


	
    _delay_us(150);
    for (i = 0; i < 8; i++){
        c >>= 1;
        if (PINC & (1 << PINC1))
            c |= (1 << 7);

        _delay_us(100);
    }

 	//	debug_putchar(c);
 		
	  debug_buf[debug_buf_in] = c;
    debug_buf_in = (debug_buf_in + 1) % DEBUG_BUF_SIZE;
}

int debug_getchar(void)
{
		int c;

    while (debug_buf_in == debug_buf_out) {};
    
    c = debug_buf[debug_buf_out];
    debug_buf_out = (debug_buf_out + 1) % DEBUG_BUF_SIZE;

    if (c == '\r'){
        c = '\n';
    }
    //debug_putchar(c);
    return c;
}

int debug_putchar(char c)
{
		cli();
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
    sei();
    return c;
}
