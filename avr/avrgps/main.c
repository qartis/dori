#include <avr/wdt.h>
#include <ctype.h>
#include <stdio.h>

#include "uart.h"
#include "nmea.h"

#define BUFLEN 80+2+1 /* 80 chars, plus possible \r\n, plus NULL */

int main(void)
{
    char buf[BUFLEN];
    uint8_t buflen = 0;

    wdt_disable();

    uart_init(BAUD(9600));

    for (;;) {
        if (buflen > sizeof(buf)) {
            //wtf, nmea shouldn't be this long. drop it
            buflen = 0;
            continue;
        }
        int c = getchar();
        if (!isprint(c) && c != '\n') {
            continue;
        }
        if (c == '\n') {
            if (buflen > 0) {
                buf[buflen] = '\0';
                parse_nmea(buf);
            }
            buflen = 0;
            continue;
        }
        buf[buflen++] = c;
    }
}
