#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
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

void uart_init(uint16_t ubrr){
    // set default baud rate 
    UART0_UBRR_HIGH =  ubrr >> 8;  
    UART0_UBRR_LOW	=  ubrr;  

    // enable receive, transmit, no interrupts
    UART0_CONTROL = (1<<RXEN0)|(1<<TXEN0);//|(1<<RXCIE0);
    // set default format, asynch, n,8,1
#ifdef URSEL0
    UCSR0C = (1<<URSEL0)|(3<<UCSZ00);
#else
    UCSR0C = (3<<UCSZ00);
#endif 
    //UCSRC = (1 << URSEL) | (1 << USBS) | (3 << UCSZ0);
}

int uart_getchar() {
    while ( !(UCSR0A & (1<<RXC0)) );
    return UDR0;
}

int uart_putchar(char data){
    while (!(UART0_STATUS & (1<<UDRE0)));
    UART0_DATA = data;
    return data;
}

void uart_print(const char *str) {
    while(*str) {
        uart_putchar(*str);
        str++;
    }
}

void uart_printint(uint16_t num){
    char buf[5];
    uint8_t i = 4;
    for(; num ; i--, num /= 10)
        buf[i] = (num % 10) + '0';

    for(; ++i<sizeof(buf);)
        uart_putchar(buf[i]);
}
