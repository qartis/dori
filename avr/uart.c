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
#endif

#ifndef DEBUG
static FILE mystdout = FDEV_SETUP_STREAM(
    (int (*)(char,FILE*))uart_putchar,
    (int (*)(FILE*))uart_getchar,
    _FDEV_SETUP_RW);
#endif

#ifndef UART_CUSTOM_INTERRUPT
/* must be 2^n */
volatile uint8_t uart_ring[UART_BUF_SIZE];
volatile uint8_t ring_in;
volatile uint8_t ring_out;
#endif

/* must be 2^n */
volatile uint8_t uart_tx_ring[UART_TX_BUF_SIZE];
volatile uint8_t tx_ring_in;
volatile uint8_t tx_ring_out;

#ifndef UART_CUSTOM_INTERRUPT
uint8_t bytes_in_ring(void)
{
    if (ring_in > ring_out)
        return (ring_in - ring_out);
    else if (ring_out > ring_in)
        return (UART_BUF_SIZE - (ring_out - ring_in));
    else
        return 0;
}
#endif

ISR(USART_UDRE_vect)
{
    uint8_t c;

    if (tx_ring_in != tx_ring_out) {
        c = uart_tx_ring[tx_ring_out];
        tx_ring_out++;
        tx_ring_out %= UART_TX_BUF_SIZE;
        UDR0 = c;
    } else {
        UCSR0B &= ~(1 << UDRIE0);
    }
}

void uart_tx_empty(void)
{
    tx_ring_in = tx_ring_out;
}

void uart_tx_flush(void)
{
    while (tx_ring_in != tx_ring_out) {};
}

#ifndef UART_CUSTOM_INTERRUPT
ISR(USART_RX_vect)
{
    uint8_t c;
    
    c = UDR0;

    uart_ring[ring_in] = c;
    ring_in++;
    ring_in %= UART_BUF_SIZE;

    if (c == '\r') {
        c = '\n';

    /* if the buffer contains a full line */
    if (c == '\n') {
        irq_signal |= IRQ_UART;
    }

#ifdef ECHO
    uart_putchar(data);
#endif
}
#endif

void uart_init(uint16_t ubrr)
{
    UBRR0H = ubrr >> 8;
    UBRR0L = ubrr;

    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0) | (1 << UDRIE0);

#ifndef DEBUG
    stdout = &mystdout;
    stdin = &mystdout;
#endif
}

#ifndef UART_CUSTOM_INTERRUPT
int uart_getchar(void)
{
   char c;

#ifndef ICRNL
ignore:
#endif
    while (ring_in == ring_out) {};

    c = uart_ring[ring_out];
    ring_out++;
    ring_out %= UART_BUF_SIZE;

    if (c == '\r') {
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

#else /* UART_CUSTOM_INTERRUPT */

int uart_getchar(void)
{
    return 0;
}
#endif

int uart_putchar(char c)
{
#ifdef ONLCR
again:
#endif

    ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
        if (tx_ring_in == tx_ring_out) {
            UCSR0B |= (1 << UDRIE0);
        }

        uart_tx_ring[tx_ring_in] = c;
        tx_ring_in++;
        tx_ring_in %= UART_TX_BUF_SIZE;
    }

#ifdef ONLCR
    if (c == '\n'){
        c = '\r';
        goto again;
    }
#endif

    return 0;
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

void putchar_wait(char c)
{
again:
    while (!(UCSR0A & (1<<UDRE0))) {};
    UDR0 = c;

    if (c == '\n') {
        c = '\r';
        goto again;
    }
}

void printwait_P(const char * PROGMEM fmt, ...)
{
    uint8_t i;
    va_list ap;
    char buf[64];

    va_start(ap, fmt);
    vsnprintf_P(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    for (i = 0; i < strlen(buf); i++)
        putchar_wait(buf[i]);
}

/*
void print_P(const char * PROGMEM s)
{
    uint8_t intr = SREG & (1 << SREG_I);

    if (intr)
        printintr_P(s);
    else
        printwait_P(s);
}
*/
