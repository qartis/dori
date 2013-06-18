#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "node.h"

void uart_irq(void)
{
}

void periodic_irq(void)
{
}

void can_irq(void)
{
}

void main(void)
{
    NODE_INIT();

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
