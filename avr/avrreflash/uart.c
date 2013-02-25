#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

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
}

int uart_getchar() {
    while ( !(UCSR0A & (1<<RXC0)) );
    return UDR0;
}

int uart_putchar(char data){
    UDR0 = data;
    while (!(UCSR0A & (1<<TXC0)));
    UCSR0A |= 1 << TXC0;
    return data;
}

void uart_print(const char *str) {
    while(*str) {
        uart_putchar(*str);
        str++;
    }
}

void uart_printhex(uint16_t num){
    char buf[5];
    buf[0] = buf[1] = buf[2] = buf[3] = '0';
    buf[4] = '\0';
    int8_t i;

    for(i = 3; i >= 0; i--) {
        char c;
        uint8_t bits = num & 0x0F;
        if(bits > 9) {
            c = bits - 0xa + 'a';
        }
        else {
            c = bits + '0';
        }

        buf[i] = c;
        num >>= 4;
    }

    uart_print(buf);
}
