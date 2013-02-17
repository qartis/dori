#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "uart.h"

#define ICRNL
#define OCRNL
#define ECHO
//#define COOKED_STDIO

#ifndef UBRR0H
#define UBRR0H UBRRH
#define UBRR0L UBRRL
#define UCSR0A UCSRA
#define UCSR0B UCSRB
#define UCSR0C UCSRC
#define UDR0 UDR
#define UDRIE0 UDRIE
#define TXEN0 TXEN
#define RXEN0 RXEN
#define RXCIE0 RXCIE
#define UCSZ00 UCSZ0
#define UDRE0 UDRE
#define USART_RX_vect USART_RXC_vect
#endif

#define ATMEGA_USART
#define UART0_UBRR_HIGH 	UBRR0H
#define UART0_UBRR_LOW 	UBRR0L
#define UART0_STATUS		UCSR0A
#define UART0_CONTROL  	UCSR0B
#define UART0_DATA     	UDR0
#define UART0_UDRIE    	UDRIE0

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

#define BUFFER_SIZE		16
volatile uint8_t uart_ring[BUFFER_SIZE];
volatile uint8_t ring_in;
volatile uint8_t ring_out;

uint8_t bytes_in_ring(void) {
    if (ring_in > ring_out)
        return (ring_in - ring_out);
    else if (ring_out > ring_in) 			
        return (BUFFER_SIZE - (ring_out - ring_in));
    else
        return 0;  		
}

ISR(USART_RX_vect){
    uart_ring[ring_in] = UART0_DATA;
    ring_in = (ring_in + 1) % BUFFER_SIZE;
}


void uart_init(uint16_t ubrr){
    ring_in = 0;
    ring_out = 0;

    // set default baud rate 
    UART0_UBRR_HIGH =  ubrr >> 8;  
    UART0_UBRR_LOW	=  ubrr;  

    // enable receive, transmit and enable receive interrupts 
    UART0_CONTROL = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
    // set default format, asynch, n,8,1
#ifdef URSEL0
    UCSR0C = (1<<URSEL0)|(3<<UCSZ00);
#else
    UCSR0C = (3<<UCSZ00);
#endif 
    //UCSRC = (1 << URSEL) | (1 << USBS) | (3 << UCSZ0);

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

int uart_getchar(FILE *f) {
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
    putchar(c);
#endif
    return c;
}

uint8_t uart_haschar(void){
    return (ring_in != ring_out);
}

int uart_putchar(char data, FILE *f){
#ifdef OCRNL
again:
#endif
    while (!(UART0_STATUS & (1<<UDRE0))){ }
    UART0_DATA = data;
#ifdef OCRNL
    if (data == '\n'){
        data = '\r';
        goto again;
    }
#endif
    return data;
}

#ifdef COOKED_STDIO
char *fgets(char *s, int bufsize, FILE *f) {
    char *p;
    int c;

    p = s;

    for (p = s; p < s + bufsize-1;){
        c = getchar();
        if (c == 0x1b){
            return s;
        }
        if (c == '\r'){
            c = '\n';
        }
        switch (c){
        case EOF:
            break;

        case '\b': /* backspace */
        case 127:  /* delete */
            if (p > s){
                *p-- = '\0';
                /*
                putchar('\b');
                putchar(' ');
                putchar('\b');
                */
            }
            break;
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

void print(uint16_t num){
    if (!num) {
        putchar('0');
        return;
    }
    char buf[5];
    uint8_t i = 4;
    for(; num ; i--, num /= 10)
        buf[i] = (num % 10) + '0';

    for(; ++i<sizeof(buf);)
        putchar(buf[i]);
}

#if 0
void printx(uint16_t num){
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

void print32(uint32_t num){
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
#endif

#if 1
void printb(uint16_t number, uint8_t base){
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
