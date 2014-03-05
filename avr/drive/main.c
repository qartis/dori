#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
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
    char buf[128];
    int32_t goal;
    uint8_t rc;

    printf("uart irq\n");

    fgets(buf, sizeof(buf), stdin);

    rc = sscanf(buf, " %ld", &goal);
    if (rc != 1)
        return 0;

    printf("got %ld\n", goal);

    motor_drive(goal, goal);

    return 0;
}

uint8_t periodic_irq(void)
{
    return 0;
}

uint8_t can_irq(void)
{
    int16_t left_ms;
    int16_t right_ms;

    switch (packet.type) {
    case TYPE_action_drive:
        left_ms = (packet.data[0] << 8) | packet.data[1];
        right_ms = (packet.data[2] << 8) | packet.data[3];

        motor_drive(left_ms, right_ms);
        break;

    default:
        break;
    }

    packet.unread = 0;

    return 0;
}

void sleep(void)
{
    sleep_enable();
    sleep_bod_disable();
    sei();
    sleep_cpu();
    sleep_disable();
}

int main(void)
{
    NODE_INIT();

    motor_init();

    sei();

    NODE_MAIN();
}
