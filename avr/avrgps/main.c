#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <ctype.h>
#include <stdio.h>

#include "uart.h"
#include "nmea.h"
#include "mcp2515.h"
#include "time.h"
#include "spi.h"

#define NMEA_LEN 80+2+1 /* 80 chars, plus possible \r\n, plus NULL */

uint8_t reinit;

void mcp2515_irq_callback(void)
{
    (void)0;
}

void periodic_callback(void)
{
    uint8_t rc;
    uint8_t buf[4];

    buf[0] = cur_time >> 24;
    buf[1] = cur_time >> 16;
    buf[2] = cur_time >> 8;
    buf[3] = cur_time >> 0;

    printf_P(PSTR("sending time\n"));
    rc = mcp2515_send(TYPE_VALUE_PERIODIC, ID_TIME, sizeof(cur_time), buf);
    if (rc) {
        reinit = 1;
        return;
    }

    /*
    _delay_ms(100);

    struct {
        int32_t lat;
        int32_t lon;
    } coord = {cur_lat, cur_lon};

    printf_P(PSTR("sending latlon\n"));
    rc = mcp2515_send(TYPE_VALUE_PERIODIC, ID_LATLON, sizeof(coord), &coord);
    if (rc) {
        reinit = 1;
        return;
    }
    */
}

void main(void)
{
    char buf[NMEA_LEN];
    uint8_t buflen;
    uint8_t rc;
    uint8_t c;

    wdt_disable();

    uart_init(BAUD(9600));
    spi_init();
    time_init();

reinit:
    reinit = 0;

    rc = mcp2515_init();
    if (rc) {
        printf_P(PSTR("mcp\n"));
        _delay_ms(1000);
        goto reinit;
    }

    for (;;) {
        printf("> ");

        buflen = 0;

        while (buflen < sizeof(buf)) {
            if (reinit) {
                goto reinit;
            }
            c = getchar();
            if (isprint(c)) {
                buf[buflen] = c;
                buflen++;
            } else if (c == '\n') {
                buf[buflen] = '\0';
                parse_nmea(buf);
                buflen = 0;
            }
        }
        /* nmea sentence was too long, so throw it away */
    }
}
