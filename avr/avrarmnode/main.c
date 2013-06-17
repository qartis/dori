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
#include "node.h"

void uart_irq(void)
{
}

void periodic_irq(void)
{
    /* this also gets called when we move the arm */
    uint8_t rc;
    uint16_t angle;
    
    angle = get_arm_angle();

    uint8_t buf[3];
    static uint8_t counter = 0;

    buf[0] = angle >> 8;
    buf[1] = angle & 0xff;
    buf[2] = counter;

    counter++;

    rc = mcp2515_send(TYPE_value_periodic, ID_arm, 3, buf);
    if (rc != 0) {
        //how to handle error here?
    }
}

void can_irq(void)
{
    uint8_t pos;

    switch (packet.type) {
    case TYPE_set_arm:
        if (packet.len != 1) {
            puts_P(PSTR("err: TYPE_set_arm data len"));
            break;
        }
        pos = packet.data[0];
        set_arm_angle(pos);
        /* send arm angle again */
        periodic_irq();
        break;

    case TYPE_get_arm:
        periodic_irq();
        break;

    default:
        break;
    }
}

void main(void)
{
    NODE_INIT();
    adc_init();

    for (;;) {
        printf_P(PSTR(">| "));

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
