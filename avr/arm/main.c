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

#include "stepper.h"
#include "can.h"
#include "irq.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "uart.h"
#include "debug.h"
#include "arm.h"
#include "adc.h"
#include "node.h"
#include "laser.h"

uint8_t send_arm_angle(uint8_t type)
{
    uint8_t rc;
    uint16_t angle;
    uint8_t buf[3];
    static uint8_t counter = 0;

    angle = get_arm_angle();

    buf[0] = angle >> 8;
    buf[1] = angle & 0xff;
    buf[2] = counter;

    counter++;

    rc = mcp2515_send_sensor(type, ID_arm, buf, 3, SENSOR_arm);
    if (rc != 0)
        puts_P(PSTR("err: arm: mcp2515_send_sensor"));

    return rc;
}

uint8_t send_stepper_angle(uint8_t type)
{
    uint8_t rc;
    uint16_t angle;
    uint8_t buf[3];
    static uint8_t counter = 0;

    angle = get_stepper_angle();

    buf[0] = angle >> 8;
    buf[1] = angle & 0xff;
    buf[2] = counter;

    counter++;

    rc = mcp2515_send_sensor(type, ID_arm, buf, 3, SENSOR_stepper);
    if (rc != 0)
        puts_P(PSTR("err: stepper: mcp2515_send_sensor"));

    return rc;
}


uint8_t send_laser_dist(uint8_t type)
{
    uint8_t rc;
    uint16_t dist;
    uint8_t buf[2];

    dist = measure_once();

    if (dist == 0) {
        rc = mcp2515_send_sensor(TYPE_sensor_error, ID_arm, buf, sizeof(buf), SENSOR_laser);
        return rc;
    }

    buf[0] = (dist >> 8) & 0xFF;
    buf[1] = (dist >> 0) & 0xFF;

    rc = mcp2515_send_sensor(type, ID_arm, buf, sizeof(buf), SENSOR_laser);
    return rc;
}

uint8_t periodic_irq(void)
{
    return 0;

    uint8_t rc;

    rc = send_arm_angle(TYPE_value_periodic);
    if (rc != 0)
        return rc;

    rc = send_stepper_angle(TYPE_value_periodic);

    return rc;
}

uint8_t process_value_request(void)
{
    uint8_t rc = 0;

    switch (packet.sensor) {
    case SENSOR_arm:
        rc = send_arm_angle(TYPE_value_explicit);
        break;
    case SENSOR_stepper:
        rc = send_stepper_angle(TYPE_value_explicit);
        break;
    case SENSOR_laser:
        rc = send_laser_dist(TYPE_value_explicit);
        break;
    default:
        break;
    }

    return rc;
}

uint8_t can_irq(void)
{
    uint8_t rc = 0;
    uint16_t val;

    switch (packet.type) {
    case TYPE_value_request:
        rc = process_value_request();
        break;
    case TYPE_action_arm_angle:
        val = packet.data[0];
        set_arm_percent(val);
        break;
    case TYPE_action_arm_spin:
        val = (packet.data[0] << 8) |
              (packet.data[1] & 0xff);
        set_stepper_angle(val);
        break;
    default:
        break;
    }

    packet.unread = 0;
    return rc;
}

uint8_t debug_irq(void)
{
    uint8_t pos;
    char buf[DEBUG_BUF_SIZE];
    uint16_t dist;

    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf)-1] = '\0';

    switch (buf[0]) {
    case 'o':
        laser_on();
        break;
    case 'f':
        laser_off();
        break;
    case 'm':
        dist = measure_once();

        if (dist == 0)
            printf("laser error\n");
        else
            printf("dist: %umm\n", dist);

        break;
    case 'a':
        pos = atoi(buf + 1);
        set_arm_percent(pos);
        break;

    }

    return 0;
}

int main(void)
{
    NODE_INIT();
    adc_init();
    laser_init();

    DDRC |= (1 << PORTC2);
    PORTC |= (1 << PORTC2);

    stepper_init();

    sei();

    NODE_MAIN();
}
