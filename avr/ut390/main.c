#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <util/atomic.h>

#define UART_BAUD 115200
#define DEBUG

#include "irq.h"
#include "debug.h"
#include "time.h"
#include "uart.h"
#include "spi.h"
//#include "mcp2515.h"
#include "node.h"
#include "can.h"
#include "laser.h"

inline uint8_t streq(const char *a, const char *b)
{
    return strcmp(a,b) == 0;
}

volatile int dist_mm;
volatile uint8_t dist_flag;

int strstart_P(const char *s1, const char * PROGMEM s2)
{
    return strncmp_P(s1, s2, strlen_P(s2)) == 0;
}

int parse_dist_str(const char *buf)
{
    char *comma;
    int dist;

    if (!strstart_P(buf, PSTR("Dist: ")))
        return -1;

    comma = strchr(buf, ',');
    if (comma == NULL)
        return -1;

    *comma = '\0';

    dist = atoi(buf + strlen("Dist: "));
    return dist;
}

extern volatile uint8_t uart_ring[UART_BUF_SIZE];
extern volatile uint8_t ring_in;
extern volatile uint8_t ring_out;

#define UDR UDR0

ISR(USART_RX_vect)
{
    uint8_t data = UDR;

    if(dist_flag)
        return;

    if (data == '\r')
        return;

    uart_ring[ring_in] = data;
    ring_in = (ring_in + 1) % UART_BUF_SIZE;
    uart_ring[ring_in] = '\0';

    /* if the buffer contains a full line */
    if (data != '\n') {
        return;
    }


    if(ring_in < strlen("Dist: ")) {
        ring_in = ring_out = 0;
        return;
    }

    dist_mm = parse_dist_str((const char*)uart_ring);
    ring_in = ring_out = 0;
    dist_flag = 1;
}

void turn_off(void)
{
    printf("turning off\n");
    printf("spamming r's\n");
    // turn off the laser in case it was in the frozen state
    uart_putchar('r');
    _delay_ms(50);
    uart_putchar('r');
    _delay_ms(50);
    uart_putchar('r');
    _delay_ms(50);


    // cancel any mode the laser is in
    printf("pressing off\n");
    PRESS(OFF_BTN);

    // turn the device off if it is on
    printf("holding off\n");
    HOLD(OFF_BTN);

    printf("done turning off\n");
}

void turn_on(void)
{
    printf("turning on\n");
    // turn on the device
    HOLD(ON_BTN);
    printf("done holding on button\n");
}

void turn_on_safe(void)
{
    printf("safely turning on\n");
    turn_off();
    // turn on the device
    printf("holding on button to turn on device\n");
    HOLD(ON_BTN);
}


// assumes laser is already on
void measure(void) {
    printf("measuring\n");
    dist_flag = 0;
    ring_out = ring_in = 0;

    printf("pressing on turn on the laser\n");
    PRESS(ON_BTN);

    printf("pressing on to take a reading\n");
    PRESS(ON_BTN);

    while(!dist_flag);
    printf("dist: %d\n", dist_mm);
}

void turn_on_and_measure(void) {
    turn_on();
    measure();
    turn_off();
}

void turn_on_and_measure_safe(void) {
    turn_on_safe();
    measure();
    turn_off();
}


uint8_t debug_irq(void)
{
    char buf[64];
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf)-1] = '\0';

    if(streq(buf, "om")) {
        turn_on_and_measure();
    } else if(streq(buf, "som")) {
        turn_on_and_measure();
    } else if(streq(buf, "on")) {
        turn_on();
    } else if(streq(buf, "son")) {
        turn_on_safe();
    } else if(streq(buf, "off")) {
        turn_off();
    } else if(streq(buf, "m")) {
        measure();
    } else {
        printf("unrecognized input: '%s'\n", buf);
    }

    debug_flush();

    _delay_ms(500);
    PROMPT;

    return 0;
}

uint8_t can_irq(void)
{
    return 0;
}

uint8_t uart_irq(void)
{
    return 0;
}

uint8_t periodic_irq(void)
{
    return 0;
}

void main(void)
{
    // make pin D5 an input pin first
    DDRD &= ~(1 << PIND5);
    // no pullup
    PORTD = 0;

    //NODE_INIT();
    sei();

    //NODE_MAIN();
}
