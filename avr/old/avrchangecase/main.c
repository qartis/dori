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

volatile uint8_t has_packet;

uint8_t from_hex(char a)
{
    if (isupper(a)) {
        return a - 'A' + 10;
    } else if (islower(a)) {
        return a - 'a' + 10;
    } else {
        return a - '0';
    }
}

void periodic_callback(void)
{
    (void)0;
}

void mcp2515_irq_callback(void)
{
    uint8_t i;

/*
    printf_P(PSTR("Case switcher received [%x %x] %db: "), packet.type, packet.id, packet.len);
    for (i = 0; i < packet.len; i++) {
        printf_P(PSTR("%x,"), packet.data[i]);
        packet.data[i] ^= 1 << 7;
    }
    printf_P(PSTR("\n"));
    */
    has_packet = 1;
}

void main(void)
{
    int buflen = 64;
    char buf[buflen];
    uint8_t i = 0;

    uart_init(BAUD(38400));
    spi_init();

    _delay_ms(200);
    printf("system start\n");

    while (mcp2515_init()) {
        printf_P(PSTR("mcp: 1\n"));
        _delay_ms(500);
    }

    sei();

    uint8_t sendbuf[10] = {0};
    uint8_t type = 0;
    uint8_t id = 0;
    uint8_t rc;

    for (;;) {
        if (has_packet) {
            for (i = 0; i < packet.len; i++) {
                packet.data[i] ^= 'A' ^ 'a';
            }
            mcp2515_send(packet.type, packet.id, packet.len, (void *)(&packet.data));
            printf("sent!\n");
            _delay_ms(10);
            has_packet = 0;
        }
        continue;
        printf_P(PSTR("case switcher > "));

        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf)-1] = '\0';

        if (strncmp(buf, "send ", strlen("send ")) == 0) {
            char *ptr = buf + strlen("send ");

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

            rc = mcp2515_send(type, id, i, sendbuf);
            printf_P(PSTR("mcp2515_send: %d\n"), rc);
        } else if (buf[0] == '.' && buf[1] == '\0') {
            rc = mcp2515_send(type, id, i, sendbuf);
            printf_P(PSTR("mcp2515_send: %d\n"), rc);
        } else {
            printf_P(PSTR("send type id [02 ff ab ..]\n"));
        }
    }
}
