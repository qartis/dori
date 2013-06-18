#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "powershot.h"
#include "uart.h"
#include "node.h"
#include "mcp2515.h"
#include "time.h"
#include "spi.h"
#include "node.h"

volatile uint8_t got_packet;
volatile uint8_t packet_buf[8];
volatile uint8_t packet_len;
volatile uint16_t packet_id;

inline uint8_t streq(const char *a, const char *b)
{
    return strcmp(a,b) == 0;
}

void periodic_irq(void)
{
}

void can_irq(void)
{
}

void uart_irq(void)
{
    char buf[64];

    uart_getbuf(buf);

    int pin = buf[1] - '0';

    if (0) {
    } else if (streq(buf, "auto")) {
        mode_auto();
    } else if (streq(buf, "manual")) {
        mode_auto();
        LOW(MODE1);
    } else if (streq(buf, "portrait")) {
        mode_auto();
        LOW(MODE2);
    } else if (streq(buf, "video")) {
        mode_auto();
        LOW(MODE3);
    } else if (streq(buf, "night")) {
        mode_auto();
        LOW(MODE4);
    } else if (streq(buf, "program")) {
        mode_auto();
        LOW(MODE6);
    } else if (streq(buf, "av")) {
        mode_auto();
        LOW(MODE1);
        LOW(MODE2);
    } else if (streq(buf, "landscape")) {
        mode_auto();
        LOW(MODE2);
        LOW(MODE4);
    } else if (streq(buf, "stitch")) {
        mode_auto();
        LOW(MODE3);
        LOW(MODE6);
    } else if (streq(buf, "nightsnap")) {
        mode_auto();
        LOW(MODE4);
        LOW(MODE6);
    } else if (streq(buf, "shutterspeed")) {
        mode_auto();
        LOW(MODE1);
        LOW(MODE2);
        LOW(MODE6);
    } else if (streq(buf, "snap")) {
        LOW(SNAP);
        _delay_ms(750);
        HIZ(SNAP);
    } else if (streq(buf, "focus")) {
        LOW(FOCUS);
        _delay_ms(500);
        HIZ(FOCUS);
    } else if (streq(buf, "zi")) {
        LOW(ZOOMIN);
        _delay_ms(50);
        HIZ(ZOOMIN);
    } else if (streq(buf, "zo")) {
        LOW(ZOOMOUT);
        _delay_ms(50);
        HIZ(ZOOMOUT);
    } else if (streq(buf, "up") || streq(buf, "\x1b\x5b\x41")) {
        LOW(UP);
        _delay_ms(200);
        HIZ(UP);
    } else if (streq(buf, "down") || streq(buf, "\x1b\x5b\x42")) {
        LOW(DOWN);
        _delay_ms(200);
        HIZ(DOWN);
    } else if (streq(buf, "left") || streq(buf, "\x1b\x5b\x44")) {
        LOW(LEFT);
        _delay_ms(200);
        HIZ(LEFT);
    } else if (streq(buf, "right") || streq(buf, "\x1b\x5b\x43")) {
        LOW(RIGHT);
        _delay_ms(200);
        HIZ(RIGHT);
    } else if (streq(buf, "disp")) {
        LOW(DISP);
        _delay_ms(200);
        HIZ(DISP);
    } else if (streq(buf, "menu")) {
        LOW(MENU);
        _delay_ms(200);
        HIZ(MENU);
    } else if (streq(buf, "func") || buf[0] == '\0') {
        LOW(FUNC);
        _delay_ms(200);
        HIZ(FUNC);
    } else if (streq(buf, "led")) {
        PINB &= ~(1 << PORTB4);
        if (PINB & PORTB4) puts("led on");
        else puts("led off");
    } else if (streq(buf, "on")) {
        HI(POWER);
        _delay_ms(500);
        HIZ(POWER);
    } else {
        puts_P(PSTR("unknown command"));
    }
}

int main(void) {
    NODE_INIT();

    //PORTD2 power
    DDRD = 0;
    DDRB = 0;
    DDRC = 0;

    PORTD = 0;
    PORTB = 0;
    PORTC = 0;

    for (;;) {
        printf_P(PSTR(XSTR(MY_ID) "> "));

        while (irq_signal == 0) {};

        if (irq_signal & IRQ_CAN) {
            can_irq();
            irq_signal &= ~IRQ_CAN;
        }

        if (irq_signal & IRQ_TIMER) {
            periodic_irq();
            irq_signal &= ~IRQ_TIMER;
        }

        if (irq_signal & IRQ_UART) {
            uart_irq();
            irq_signal &= ~IRQ_UART;
        }
    }
}
