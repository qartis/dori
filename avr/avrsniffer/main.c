#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "uart.h"
#include "spi.h"
#include "mcp2515.h"

uint8_t from_hex(char a){
    if (isupper(a)) {
        return a - 'A' + 10;
    } else if (islower(a)) {
        return a - 'a' + 10;
    } else {
        return a - '0';
    }
}

void mcp2515_irq_callback(void)
{
    uint8_t i;

    printf_P(PSTR("Sniffer received [%x %x] %db: "), packet.type, packet.id, packet.len);
    for (i = 0; i < packet.len; i++) {
        printf_P(PSTR("%x,"), packet.data[i]);
    }
    printf_P(PSTR("\n"));
}

void main(void)
{
    int buflen = 64;
    char buf[buflen];
    uint8_t i;

    uart_init(BAUD(38400));
    spi_init();

    while (mcp2515_init()) {
        printf_P(PSTR("mcp: 1\n"));
        _delay_ms(500);
    }

    sei();

    for (;;) {
        printf_P(PSTR("> "));

        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf)-1] = '\0';

        if (strncmp(buf, "send ", strlen("send ")) == 0) {
            char *ptr = buf + strlen("send ");
            uint8_t sendbuf[10];
            uint8_t type;
            uint8_t id;

            type = 16 * from_hex(ptr[0]) + from_hex(ptr[1]);
            ptr += strlen("ff ");
            id = 16 * from_hex(ptr[0]) + from_hex(ptr[1]);
            ptr += strlen("ff ");

            i = 0;
            while (*ptr && i < 8) {
                sendbuf[i] = 16 * from_hex(ptr[0]) + from_hex(ptr[1]);
                i++;
                if (ptr[2] == '\0') {
                    break;
                }
                ptr += strlen("ff ");
            }

            uint8_t rc = mcp2515_send(type, id, i, sendbuf);

            printf_P(PSTR("mcp2515_send: %d\n"), rc);
        } else {
            printf_P(PSTR("send type id [02 ff ab ..]\n"));
        }
    }
}
