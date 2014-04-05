#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "irq.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "node.h"
#include "free_ram.h"
#include "can.h"
#include "ircam.h"
#include "debug.h"

ISR(USART_RX_vect)
{
    uint8_t c;

    c = UDR0;

    if (read_flag)
        return;

    if (read_size < sizeof(rcv_buf) - 1)
        rcv_buf[read_size++] = c;

    if (read_size < target_size)
        return;

    read_flag = 1;
}

uint8_t user_irq(void)
{
    return 0;
}

uint8_t can_irq(void)
{
    uint8_t rc;
    uint8_t uptime_buf[4];

    rc = 0;

    switch (packet.type) {
    case TYPE_ircam_read:
        IRCAM_ON();

        _delay_ms(1000);

        rc = ircam_init_xfer();
        if(rc == 0) {
            rc = ircam_read_fbuf();
            if(rc) {
                mcp2515_send_sensor(TYPE_sensor_error, MY_ID, packet.sensor, (uint8_t*)&rc, sizeof(rc));
                break;
            }
            printf("read_fbuf rc = %d\n", rc);
        }

        IRCAM_OFF();

        break;
    case TYPE_ircam_reset:
        ircam_reset();

        break;
    case TYPE_value_request:
        if (packet.sensor == SENSOR_uptime) {
            uptime_buf[0] = uptime >> 24;
            uptime_buf[1] = uptime >> 16;
            uptime_buf[2] = uptime >> 8;
            uptime_buf[3] = uptime >> 0;

            mcp2515_send_sensor(TYPE_value_explicit, MY_ID, SENSOR_uptime, uptime_buf, sizeof(uptime_buf));
        } else {
            mcp2515_send_sensor(TYPE_sensor_error, MY_ID, packet.sensor, NULL, 0);
        }
        break;
    }

    packet.unread = 0;

    return rc;
}

uint8_t periodic_irq(void)
{
    return 0;
}

uint8_t debug_irq(void)
{
    char buf[32];

    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf)-1] = '\0';

    debug_flush();

    if (strcmp(buf, "on") == 0) {
        PORTD |= (1 << PORTD6);
    } else if (strcmp(buf, "off") == 0) {
        PORTD &= ~(1 << PORTD6);
    } else if (strcmp(buf, "snap") == 0) {
        IRCAM_ON();
        _delay_ms(500);
        ircam_init_xfer();
        ircam_read_fbuf();
        IRCAM_OFF();
    } else if (strcmp(buf, "rst") == 0) {
        ircam_reset();
    } else if (strcmp(buf, "mcp") == 0) {
        mcp2515_dump();
    } else {
        printf("got '%s'\n", buf);
    }

    PROMPT;

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

    sei();

    // For the ircam
    DDRD |= (1 << PORTD6);

    NODE_MAIN();
}
