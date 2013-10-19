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

    rc = mcp2515_send_tagged(type, ID_arm, buf, 3, TAG_get_arm);
    if (rc != 0) {
        puts_P(PSTR("err: arm: mcp2515_send_tagged"));
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

    rc = mcp2515_send_tagged(type, ID_arm, buf, 3, TAG_get_stepper);
    if (rc != 0) {
        puts_P(PSTR("err: arm: mcp2515_send_tagged"));
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

    switch(packet.tag) {
    case TAG_get_arm:
        rc = send_arm_angle(TYPE_value_explicit);
        break;
    case TAG_get_stepper:
        rc = send_stepper_angle(TYPE_value_explicit);
    default:
        break;
    }

    return rc;
}

uint8_t process_command(void)
{
    uint8_t rc = 0;

    switch(packet.tag) {
    case TAG_set_arm:
        {
            uint8_t val = packet.data[0];
            set_arm_percent(val);
            break;
        }
    case TAG_set_stepper:
        {
            uint16_t val = packet.data[0] << 8;
            val |= packet.data[1] & 0xff;
            set_stepper_angle(val);
            break;
        }
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
    case TYPE_command:
        rc = process_command();
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
