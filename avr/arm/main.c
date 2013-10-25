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
#include "stepper.h"
#include "can.h"
#include "irq.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "arm.h"
#include "node.h"

uint8_t uart_irq(void)
{
    return 0;
}

uint8_t send_arm_angle(uint8_t type)
{
    uint8_t rc;
    uint16_t angle;
    
    angle = get_arm_angle();

    uint8_t buf[3];
    static uint8_t counter = 0;

    buf[0] = angle >> 8;
    buf[1] = angle & 0xff;
    buf[2] = counter;

    counter++;

    rc = mcp2515_send_sensor(type, ID_arm, buf, 3, SENSOR_arm);
    if (rc != 0) {
        puts_P(PSTR("err: arm: mcp2515_send_sensor"));
    }

    return rc;
}

uint8_t send_stepper_angle(uint8_t type)
{
    uint8_t rc;
    uint16_t angle;
    
    angle = get_stepper_angle();

    uint8_t buf[3];
    static uint8_t counter = 0;

    buf[0] = angle >> 8;
    buf[1] = angle & 0xff;
    buf[2] = counter;

    counter++;

    rc = mcp2515_send_sensor(type, ID_arm, buf, 3, SENSOR_stepper);
    if (rc != 0) {
        puts_P(PSTR("err: stepper: mcp2515_send_sensor"));
    }

    return rc;
}


uint8_t periodic_irq(void)
{
    uint8_t rc = send_arm_angle(TYPE_value_periodic);
    if(rc != 0) return rc;

    rc = send_stepper_angle(TYPE_value_periodic);
    return rc;
}

uint8_t process_value_request(void)
{
    uint8_t rc = 0;

    switch(packet.sensor) {
    case SENSOR_arm:
        rc = send_arm_angle(TYPE_value_explicit);
        break;
    case SENSOR_stepper:
        rc = send_stepper_angle(TYPE_value_explicit);
    default:
        break;
    }

    return rc;
}

uint8_t can_irq(void)
{
    uint8_t rc = 0;

    switch (packet.type) {
    case TYPE_value_request:
        rc = process_value_request();
        break;
    case TYPE_action_arm_spin:
        {
            uint8_t val = packet.data[0];
            set_arm_percent(val);
            break;
        }
    case TYPE_action_arm_angle:
        {
            uint16_t val = packet.data[0] << 8;
            val |= packet.data[1] & 0xff;
            set_stepper_angle(val);
            break;
        }
    default:
        break;
    }

    packet.unread = 0;
    return rc;
}

void main(void)
{
    NODE_INIT();
    adc_init();

    DDRC |= (1 << PORTC2);
    PORTC |= (1 << PORTC2);

    stepper_init();

    sei();

    NODE_MAIN();
}
