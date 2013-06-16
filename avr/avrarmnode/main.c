#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "arm.h"

volatile uint8_t int_signal;
volatile struct mcp2515_packet_t p;

void periodic_callback(void)
{
    /* this also gets called when we move the arm */
    uint8_t rc;
    uint16_t angle;
    
    angle = get_arm_angle();

    rc = mcp2515_send(TYPE_value_periodic, ID_arm, 2, (void *)&angle);
    if (rc != 0) {
        //how to handle error here?
    }
}

void mcp2515_irq_callback(void)
{
    int_signal = 1;
    p = packet;
}

void bottom_half(void)
{
    uint8_t pos;

    switch (p.type) {
    case TYPE_set_arm:
        if (p.len != 1) {
            puts_P(PSTR("err: TYPE_set_arm data len"));
            break;
        }
        pos = p.data[0];
        set_arm_angle(pos);
        /* send arm angle again */
        periodic_callback();
        break;

    case TYPE_get_arm:
        periodic_callback();
        break;

    default:
        break;
    }
}

void main(void)
{
    adc_init();
    uart_init(BAUD(38400));
    spi_init();
    time_init();

    _delay_ms(200);
    puts_P(PSTR("\n\narm node start"));

    while (mcp2515_init()) {
        puts_P(PSTR("mcp: init"));
        _delay_ms(500);
    }

    sei();

    for (;;) {
        printf_P(PSTR(">| "));

        while (int_signal == 0) {};

        int_signal = 0;
        bottom_half();
    }
}
