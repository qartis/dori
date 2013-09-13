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
#include "arm.h"
#include "node.h"

uint8_t uart_irq(void)
{
    uint16_t goal;
    scanf("%u", &goal);
    if (goal > 9) {
        uint8_t i;
        for (i = 0; i < goal; i++) {
            motor_cw();
        }
    } else {
        set_arm_angle(goal);
    }
    return 0;
}

uint8_t periodic_irq(void)
{
    /* this also gets called when we move the arm */
    uint8_t rc;
    uint16_t angle;
    
    angle = get_arm_angle();

    uint8_t buf[3];
    static uint8_t counter = 0;

    buf[0] = angle >> 8;
    buf[1] = angle & 0xff;
    buf[2] = counter;

    counter++;

    //rc = mcp2515_send(TYPE_value_periodic, ID_arm, buf, 3);
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
    case TYPE_set_arm:
        if (packet.len != 1) {
            puts_P(PSTR("err: TYPE_set_arm data len"));
            break;
        }
        pos = packet.data[0];
        set_arm_angle(pos);
        /* send arm angle again */
        periodic_irq();
        break;

    case TYPE_get_arm:
        periodic_irq();
        break;

    default:
        break;
    }

    return 0;
}

void main(void)
{
    NODE_INIT();
    adc_init();

    DDRC |= (1 << PORTC2);
    PORTC |= (1 << PORTC2);
    PORTC |=  (1 << PORTC0);
    PORTC |=  (1 << PORTC1);

    motor_init();

    sei();

    NODE_MAIN();
}
