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
#include "errno.h"
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

uint8_t laser_cb(uint16_t dist)
{
    uint8_t i;
    uint8_t rc;
    uint8_t buf[4];

    buf[0] = stepper_state >> 8;
    buf[1] = stepper_state & 0xff;
    buf[2] = dist >> 8;
    buf[3] = dist & 0xff;

    rc = mcp2515_xfer(TYPE_xfer_chunk, MY_ID, SENSOR_laser, buf, sizeof(buf));
    if (rc) {
        return rc;
    }

    for (i = 0; i < 255; i++) {
        stepper_cw();
    }

    return 0;
}

uint8_t laser_do_sweep(uint16_t start_angle, uint16_t end_angle,
        uint16_t stepsize)
{
    uint8_t rc;
    uint8_t buf[6];

    rc = stepper_set_angle(start_angle);
    if (rc) {
        printf("laser_sweep: set_angle %d\n", rc);
        return rc;
    }

    rc = stepper_set_stepsize(stepsize);
    if (rc) {
        return rc;
    }

    if (start_angle >= end_angle) {
        return ERR;
    }

    buf[0] = start_angle >> 8;
    buf[1] = start_angle & 0xff;
    buf[2] = end_angle >> 8;
    buf[3] = end_angle & 0xff;
    buf[4] = stepsize >> 8;
    buf[5] = stepsize & 0xff;

    mcp2515_xfer_begin();

    rc = mcp2515_xfer(TYPE_laser_sweep_header, MY_ID, SENSOR_laser,
            buf, sizeof(buf));
    if (rc) {
        return rc;
    }

    stepper_wake();

    laser_sweep(laser_cb);

    stepper_sleep();

    return 0;
}

uint8_t send_arm_angle(uint8_t type)
{
    uint16_t angle;
    uint8_t buf[3];

    angle = get_arm_angle();

    buf[0] = angle >> 8;
    buf[1] = angle & 0xff;

    return mcp2515_send_sensor(type, ID_arm, SENSOR_arm, buf, sizeof(buf));
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
    case TYPE_laser_sweep_begin:
        rc = laser_do_sweep(0, 18000, 1);
        if (rc != 0)
            mcp2515_send_sensor(TYPE_sensor_error, ID_arm, SENSOR_laser, &rc, sizeof(rc));
        printf("sweep rc %d\n", rc);
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
    case 'm':
        dist = measure_once();

        if (dist == 0)
            printf("laser error\n");
        else
            printf("dist: %umm\n", dist);

        buf[0] = (dist >> 8) & 0xFF;
        buf[1] = (dist >> 0) & 0xFF;

        rc = mcp2515_send_sensor(TYPE_value_explicit, ID_arm, SENSOR_laser, (uint8_t *)buf, 2);
        printf("rc %d\n", rc);

        break;
    case 'x':
        {
            uint16_t i;
            stepper_wake();
            stepper_set_state(0);
            for (i = 0; i < 255; i++) {
                stepper_ccw();
            }
            stepper_sleep();
        }
        break;

    case 'a':
        pos = atoi(buf + 1);
        set_arm_percent(pos);
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
