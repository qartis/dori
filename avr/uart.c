#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "uart.h"

#define ICRNL
#define ONLCR
#define ECHO
//#define COOKED_STDIO

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

static FILE mystdout = FDEV_SETUP_STREAM(
    (int (*)(char,FILE*))uart_putchar, 
    (int (*)(FILE*))uart_getchar,
    _FDEV_SETUP_RW);

#define BUFFER_SIZE		64
static volatile uint8_t uart_ring[BUFFER_SIZE];
static volatile uint8_t ring_in;
static volatile uint8_t ring_out;

#ifndef UART_CUSTOM_INTERRUPT
ISR(USART_RXC_vect)
{
    uart_ring[ring_in] = UDR;
    ring_in = (ring_in + 1) % BUFFER_SIZE;
}
#endif

#if 0
static uint8_t bytes_in_ring(void)
{
    if (ring_in > ring_out)
        return (ring_in - ring_out);
    else if (ring_out > ring_in) 			
        return (BUFFER_SIZE - (ring_out - ring_in));
    else
        return 0;  		
}
#endif

void uart_init(uint16_t ubrr)
{
    UBRRH = ubrr >> 8;  
    UBRRL = ubrr;  

    UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
#ifdef __AVR_ATmega32__
    UCSRC = (1 << URSEL) | (1 << USBS) | (3 << UCSZ0);
#endif

    stdout = &mystdout;
    stdin = &mystdout;

    sei();
}


#if 0
extern volatile uint8_t timeout_counter;
int getc_timeout(uint8_t sec) {
    uint8_t c;

    while (ring_in == ring_out && timeout_counter < sec){ }

    if (timeout_counter >= sec){
        return EOF;
    }

    c = uart_ring[ring_out];
    ring_out = (ring_out + 1) % BUFFER_SIZE;

#ifdef ECHO
    putchar(c);
#endif

    return (int)c;
}
#endif

int uart_getchar(void)
{
   int c;

#ifndef ICRNL
ignore:
#endif
    while (ring_in == ring_out){ }

    c = uart_ring[ring_out];
    ring_out = (ring_out + 1) % BUFFER_SIZE;

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

int getc_wait(volatile uint8_t *signal)
{
   int c;

#ifndef ICRNL
ignore:
#endif
    while (ring_in == ring_out && *signal == 0){ }

    if (*signal)
        return EOF;

    c = uart_ring[ring_out];
    ring_out = (ring_out + 1) % BUFFER_SIZE;

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

char *getline(char *s, int bufsize, FILE *f, volatile uint8_t *signal)
{
    char *p;
    int c;
    uint8_t dummy_signal = 0;

    if (signal == NULL)
        signal = &dummy_signal;

    for (p = s; p < s + bufsize - 1; ) {
        c = getc_wait(signal);
        if (c == EOF)
            return NULL;

        if (c == 0x1b)
            break;

        switch (c) {
        case EOF:
            break;

        case '\b': /* backspace */
        case 127:  /* delete */
            if (p > s){
                *p-- = '\0';
                putchar('\b');
                putchar(' ');
                putchar('\b');
            }
            break;
        
        case '\r':
        case '\n':
            goto done;

        default:
            *p++ = c;
            break;
        }
    }

done:
    *p = '\0';

    return s;
}

#ifdef COOKED_STDIO
char *fgets(char *s, int bufsize, FILE *f)
{
    char *p;
    int c;

    p = s;

    for (p = s; p < s + bufsize-1;){
        c = getchar();
        if (c == 0x1b){
            break;
        }

        switch (c){
        case EOF:
            break;

        case '\b': /* backspace */
        case 127:  /* delete */
            if (p > s){
                *p-- = '\0';
                putchar('\b');
                putchar(' ');
                putchar('\b');
            }
            break;

        case '\r':
            c = '\n';
            /* fall through */

        default:
            *p++ = c;
            break;
        }

        if (c == '\n'){
            break;
        }
    }
    *p = '\0';

    return s;
}
#endif

void printu(uint16_t num)
{
    char buf[5];
    uint8_t i = 4;

    if (num == 0) {
        putchar('0');
        return;
    }

    do {
        buf[i] = (num % 10) + '0';
        i--;
        num /= 10;
    } while (num);

    i++;

    do {
        putchar(buf[i]);
    } while (++i < sizeof(buf));
}

void prints(int16_t num)
{
    if (num > 0) {
        printu(num);
    } else {
        putchar('-');
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

#if 0
void printx(uint16_t num)
{
    if (!num) {
        putchar('0');
        return;
    }
    char buf[4];
    uint8_t i = 3;
    for(; num ; i--, num /= 16)
        buf[i] = "0123456789abcdef"[num % 16];

    for(; ++i<sizeof(buf);)
        putchar(buf[i]);
}

void print32(uint32_t num)
{
    if (!num) {
        putchar('0');
        return;
    }
    char buf[10];
    uint8_t i = 9;
    for(; num ; i--, num /= 10)
        buf[i] = (num % 10) + '0';

    for(; ++i<sizeof(buf);)
        putchar(buf[i]);
}

void printb(uint16_t number, uint8_t base)
{
    if (!number) {
        putchar('0');
        return;
    }
    char buf[5];
    uint8_t i = 4;
    for(; number && i ; --i, number /= base)
        buf[i] = "0123456789abcdef"[number % base];

    while (++i < sizeof(buf))
        putchar(buf[i]);
}
#endif

int puts(const char *str)
{
    do {
        putchar(*str);
        str++;
    } while (*str);

    uart_putchar('\n');

    return 0;
}
