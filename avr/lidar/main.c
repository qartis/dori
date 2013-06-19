#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>


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

    NODE_MAIN();
}
