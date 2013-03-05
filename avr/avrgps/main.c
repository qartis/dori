#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <ctype.h>
#include <stdio.h>

#include "uart.h"
#include "nmea.h"
#include "mcp2515.h"
#include "timer.h"
#include "time.h"
#include "spi.h"

#define NMEA_LEN 80+2+1 /* 80 chars, plus possible \r\n, plus NULL */

void mcp2515_irq_callback(void)
{
    (void)0;
}

void periodic_callback(void)
{
}

void main(void)
{
    char buf[NMEA_LEN];
    uint8_t buflen;
    uint8_t rc;
    uint8_t c;

    wdt_disable();

    uart_init(BAUD(38400));
    spi_init();
    time_init();

reinit:

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
