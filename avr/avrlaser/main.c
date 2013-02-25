#include <ctype.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "laser.h"
#include "time.h"

struct mcp2515_packet_t p;

int main(void) {
    int buflen = 32;
    uint8_t buf[buflen];

	uart_init(BAUD(38400));
    spi_init();
    laser_init();
    time_init();

    while (mcp2515_init() == 0) {
        printf("mcp\n");
        _delay_ms(500);
    }

    sei();

    uint8_t i = 0;
    uint8_t rxlen = 0;
    uint16_t decoded = 0;

    for (;;) {
        printf("> ");
        while (!got_packet) {
            while (uart_haschar()) {
                buf[rxlen] = getchar();
                if (rxlen == 8 || buf[rxlen] == '\n') {
                    buf[rxlen] = '\0';
                    goto uartcmd;
                }
                rxlen++;
            }
            _delay_ms(10);
        }

        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            p = packet;
            got_packet = 0;
        }

        if (p.type == TYPE_SET_TIME) {
            time = (uint32_t)buf[0] << 24 |
                   (uint32_t)buf[1] << 16 |
                   (uint32_t)buf[2] << 8  |
                   (uint32_t)buf[3] << 0;
            printf("time reset to: %lu\n", time);
        }

        printf("Received [%x %x] %db: ", p.type, p.id, p.len);
        for (i = 0; i < p.len; i++) {
            printf("%x,", p.data[i]);
        }
        printf("\n");

        continue;


uartcmd:
        if (strcmp((char *)buf, "on") == 0) {
            laser_on();
            printf("turned on\n");
            /*
        } else if (strcmp((char *)buf, "read") == 0) {
            */
        } else {
            /*
            uint16_t decoded = laser_read();
            */
            for (;;) {
                decoded++;

                buf[0] = decoded >> 8;
                buf[1] = decoded & 0xff;

                rc = mcp2515_send(TYPE_VALUE_EXPLICIT, ID_LASER, 2, buf);
                printf("mcp2515_send: %u\n", rc);
            }

            printf("sent LASER: %u\n", decoded);
        //} else {
        //    printf("unknown command\n");
        }
        rxlen = 0;
    }
}
