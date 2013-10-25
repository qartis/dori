#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#include "uart.h"
#include "irq.h"
#include "mcp2515.h"

#ifndef UART_CUSTOM_INTERRUPT
#define ICRNL
#define ONLCR
#endif
//#define ECHO

#ifdef UBRR0H
#define UBRRH UBRR0H 
#define UBRRL UBRR0L 
#define UCSRA UCSR0A 
#define UCSRB UCSR0B 
#define UCSRC UCSR0C 
#define UDR UDR0 
#define UDRIE UDRIE0 
#define TXEN TXEN0 
#define RXEN RXEN0 
#define RXCIE RXCIE0 
#define UCSZ0 UCSZ00 
#define UDRE UDRE0 
#define USART_RXC_vect USART_RX_vect 
#define URSEL URSEL0
#endif

#ifndef DEBUG
static FILE mystdout = FDEV_SETUP_STREAM(
    (int (*)(char,FILE*))uart_putchar, 
    (int (*)(FILE*))uart_getchar,
    _FDEV_SETUP_RW);
#endif

/* must be 2^n */
volatile uint8_t uart_ring[UART_BUF_SIZE];
volatile uint8_t ring_in;
volatile uint8_t ring_out;

#ifndef UART_CUSTOM_INTERRUPT
ISR(USART_RXC_vect)
{
    uint8_t data = UDR;

    uart_ring[ring_in] = data;
    ring_in = (ring_in + 1) % UART_BUF_SIZE;

    if (data == '\r')
        data = '\n';

    /* if the buffer contains a full line */
    if (data == '\n') {
        if (irq_signal & IRQ_UART) {
            puts_P(PSTR("UART OVERRUN"));
        }
        irq_signal |= IRQ_UART;
    }


#ifdef ECHO
resend:
    /* echo the char */
    while (!(UCSRA & (1<<UDRE))) {};
    UDR = data;

    if (data == '\n') {
        data = '\r';
        goto resend;
    }
#endif
}
#endif

void uart_init(uint16_t ubrr)
{
    UBRRH = ubrr >> 8;  
    UBRRL = ubrr;  

    UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);

#ifndef DEBUG
    stdout = &mystdout;
    stdin = &mystdout;
#endif
}

int uart_getchar(void)
{
   int c;

#ifndef ICRNL
ignore:
#endif
    while (ring_in == ring_out){ }

    c = uart_ring[ring_out];
    ring_out = (ring_out + 1) % UART_BUF_SIZE;

    if (c == '\r'){
#ifdef ICRNL
        c = '\n';
#else
        goto ignore;
#endif
    }
#ifdef ECHO
    uart_putchar(c);
#endif
    return c;
}

uint8_t uart_haschar(void)
{
    return (ring_in != ring_out);
}

int uart_putchar(char data)
{
#ifdef ONLCR
again:
#endif
    while (!(UCSRA & (1<<UDRE))){ }
    UDR = data;
#ifdef ONLCR
    if (data == '\n'){
        data = '\r';
        goto again;
    }
#endif
    return data;
}

void printu(uint16_t num)
{
    char buf[5];
    uint8_t i = 4;

    if (num == 0) {
        uart_putchar('0');
        return;
    }

    do {
        buf[i] = (num % 10) + '0';
        i--;
        num /= 10;
    } while (num);

    i++;

    do {
        uart_putchar(buf[i]);
    } while (++i < sizeof(buf));
}

void prints(int16_t num)
{
    if (num > 0) {
        printu(num);
    } else {
        uart_putchar('-');
        printu(-num);
    }
}

void print(const char *s)
{
    while (*s) {
        uart_putchar(*s);
        s++;
    }
}

void print_P(const char * PROGMEM s)
{
    uint8_t c;

    while ((c = pgm_read_byte(s))) {
        uart_putchar(c);
        s++;
    }
}
