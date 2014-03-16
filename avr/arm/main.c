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

    rc = mcp2515_send_sensor(type, ID_arm, SENSOR_arm, buf, 3);
    if (rc != 0)
        puts_P(PSTR("err: arm: mcp2515_send_sensor"));

    return rc;
}

uint8_t send_laser_dist(uint8_t type)
{
    uint8_t rc;
    uint16_t dist;
    uint8_t buf[2];
    uint8_t err_num;

    dist = measure_once();

    if (dist == 0) {
        err_num = __LINE__;
        rc = mcp2515_send_sensor(TYPE_sensor_error, ID_arm, SENSOR_laser, &err_num, sizeof(err_num));
        return rc;
    }

    buf[0] = (dist >> 8) & 0xFF;
    buf[1] = (dist >> 0) & 0xFF;

    rc = mcp2515_send_sensor(type, ID_arm, SENSOR_laser, buf, sizeof(buf));

    return rc;
}

uint8_t send_stepper_state(uint8_t type)
{
    uint8_t rc;
    int32_t state;
    uint8_t buf[4];

    state = stepper_get_state();

    buf[0] = (uint32_t)(state & 0xff000000) >> 24;
    buf[1] = (uint32_t)(state & 0x00ff0000) >> 16;
    buf[2] = (uint32_t)(state & 0x0000ff00) >> 8;
    buf[3] = (uint32_t)(state & 0x000000ff) >> 0;

    rc = mcp2515_send_sensor(type, ID_arm, SENSOR_stepper, buf, sizeof(buf));

    return rc;
}

uint8_t periodic_irq(void)
{
    return 0;









    return send_arm_angle(TYPE_value_periodic);
}

uint8_t process_value_request(void)
{
    uint8_t rc;

    rc = 0;

    switch (packet.sensor) {
    case SENSOR_arm:
        rc = send_arm_angle(TYPE_value_explicit);
        break;
    case SENSOR_laser:
        rc = send_laser_dist(TYPE_value_explicit);
        break;
    case SENSOR_stepper:
        send_stepper_state(TYPE_value_explicit);
        break;
    default:
        break;
    }

    return rc;
}

uint8_t can_irq(void)
{
    int32_t stepper_angle;
    uint16_t val;
    uint8_t buf[2];
    uint8_t rc;

    rc = 0;

    switch (packet.type) {
    case TYPE_value_request:
        rc = process_value_request();
        break;
    case TYPE_action_arm_angle:
        val = packet.data[0];
        rc = set_arm_percent(val);
        if (rc != 0) {
            val = get_arm_angle();
            buf[0] = val >> 8;
            buf[1] = val & 0xff;
            mcp2515_send_sensor(TYPE_sensor_error, ID_arm, SENSOR_arm, buf, sizeof(buf));
        }
        break;
    case TYPE_action_stepper_setstate:
        stepper_angle =
            (uint32_t)packet.data[0] << 24 |
            (uint32_t)packet.data[1] << 16 |
            (uint32_t)packet.data[2] << 8  |
            (uint32_t)packet.data[3] << 0;

        stepper_set_state(stepper_angle);
        break;
    case TYPE_action_stepper_angle:
        stepper_angle =
            (uint32_t)packet.data[0] << 24 |
            (uint32_t)packet.data[1] << 16 |
            (uint32_t)packet.data[2] << 8  |
            (uint32_t)packet.data[3] << 0;

        rc = stepper_set_angle(stepper_angle);
        if (rc != 0)
            mcp2515_send_sensor(TYPE_sensor_error, ID_arm, SENSOR_stepper, &rc, sizeof(rc));
        break;
    default:
        break;
    }

    packet.unread = 0;

    return rc;
}

uint8_t debug_irq(void)
{
    char buf[DEBUG_BUF_SIZE];
    uint16_t dist;
    uint8_t pos;
    uint8_t rc;

    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf)-1] = '\0';

    switch (buf[0]) {
    case '0':
        rc = measure_rapid_fire(10);
        printf("rc %d\n", rc);
        break;
    case '1':
        rc = measure_rapid_fire(100);
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
    case 'x':
        DDRC |= (1 << PORTC5);
        PORTC &= ~(1 << PORTC5);
        break;
    case 't':
        {
            int32_t new_state;
            sscanf(buf + 1, "%ld", &new_state);
            stepper_set_state(new_state);
        }
        break;
    case 'z':
        {
            int32_t new_angle;
            sscanf(buf + 1, "%ld", &new_angle);
            rc = stepper_set_angle(new_angle);
            printf("rc %u\n", rc);
        }
        break;
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
    NODE_INIT();
    adc_init();
    laser_init();
    arm_init();
    stepper_init();

    sei();

    NODE_MAIN();
}
