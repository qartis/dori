#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "uart.h"
#include "motor.h"
#include "can.h"
#include "irq.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "node.h"

uint8_t uart_irq(void)
{
    int32_t i;
    int32_t goal;
    scanf("%ld", &goal);
    motor_left(goal);
    motor_right(goal);
    return 0;
}

uint8_t periodic_irq(void)
{
    uint8_t rc;
    

    uint8_t buf[3];
    static uint8_t counter = 0;

    buf[2] = counter;

    counter++;

    rc = 0;
    if (rc != 0) {
        return rc;
    }

    return 0;
}

uint8_t can_irq(void)
{
    uint8_t pos;

    switch (packet.type) {

    default:
        break;
    }

    return 0;
}

void main(void)
{
    NODE_INIT();

    motor_init();

    sei();

    NODE_MAIN();
}
