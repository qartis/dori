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
#include "debug.h"

uint8_t debug_irq(void)
{
    return 0;
}

uint8_t uart_irq(void)
{
    return 0;




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

    while (mcp2515_get_packet(&packet) == 0) {
        switch (packet.type) {
        case TYPE_action_drive:
            left_ms = (packet.data[0] << 8) | packet.data[1];
            right_ms = (packet.data[2] << 8) | packet.data[3];

            motor_drive(left_ms, right_ms);

            mcp2515_send(TYPE_drive_done, MY_ID, NULL, 0);
            break;
        case TYPE_value_request:
            if (packet.sensor == SENSOR_uptime) {
                uint8_t uptime_buf[4];
                uptime_buf[0] = uptime >> 24;
                uptime_buf[1] = uptime >> 16;
                uptime_buf[2] = uptime >> 8;
                uptime_buf[3] = uptime >> 0;

                mcp2515_send_sensor(TYPE_value_explicit, MY_ID, SENSOR_uptime, uptime_buf, sizeof(uptime_buf));
            } else {
                mcp2515_send_sensor(TYPE_sensor_error, MY_ID, packet.sensor, NULL, 0);
            }
            break;

        default:
            break;
        }
    }

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
    node_init();

    motor_init();

    sei();

    node_main();
}
