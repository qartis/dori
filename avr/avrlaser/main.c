#include <ctype.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "laser.h"
#include "time.h"

volatile uint8_t reinit;

void periodic_callback(void)
{
    uint16_t decoded;

    printf_P(PSTR("laser: period cb\n"));
    decoded = laser_read();
    if (decoded == LASER_ERROR) {
        mcp2515_send(TYPE_SENSOR_ERROR, ID_LASER, 0, NULL);
        reinit = 1;
    } else {
        mcp2515_send(TYPE_VALUE_EXPLICIT, ID_LASER, 2, &decoded);
    }
}

void mcp2515_irq_callback(void)
{
    uint8_t i;
    uint16_t decoded;

    printf("laser rx [%x %x] %db: ", packet.type, packet.id, packet.len);
    for (i = 0; i < packet.len; i++) {
        printf("%x,", packet.data[i]);
    }
    printf("\n");

    if (packet.id != MY_ID) {
        return;
    }

    switch (packet.type) {
    case TYPE_VALUE_EXPLICIT:
        decoded = laser_read();
        if (decoded == LASER_ERROR) {
            mcp2515_send(TYPE_SENSOR_ERROR, ID_LASER, 0, NULL);
            reinit = 1;
        } else {
            mcp2515_send(TYPE_VALUE_EXPLICIT, ID_LASER, 2, &decoded);
        }

        break;
    }
}

void main(void)
{
    uint8_t rc;
    uint16_t decoded;

    wdt_disable();

    uart_init(BAUD(38400));
    spi_init();
    laser_init();
    time_init();

reinit:
    rc = mcp2515_init();
    if (rc) {
        printf_P(PSTR("mcp: error\n"));
        _delay_ms(1000);
        goto reinit;
    }

    for (;;) {
        putchar('>');

        if (reinit) {
            reinit = 0;
            goto reinit;
        }

        switch (getchar()) {
        case 'r':
            decoded = laser_read();
            if (decoded == LASER_ERROR)
                mcp2515_send(TYPE_SENSOR_ERROR, ID_LASER, 0, NULL);
            else
                mcp2515_send(TYPE_VALUE_EXPLICIT, ID_LASER, 2, &decoded);

            break;
        default:
            printf_P(PSTR("unhandled uart cmd\n"));
            break;
        }
    }
}
