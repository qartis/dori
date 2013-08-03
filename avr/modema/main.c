#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "can.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "node.h"
#include "sim900.h"
#include "debug.h"
#include "irq.h"

#define streq_P(a,b) (strcmp_P(a, b) == 0)
#define strstart(a,b) (strncmp_P(a, PSTR(b), strlen(b)) == 0)

uint8_t uart_irq(void)
{
    return 0;
}

uint8_t user_irq(void)
{
    uint8_t rc;

    printf("conn\n");
    rc = TCPConnect();
    if (rc) {
        printf("conn: %d\n", rc);

        /* don't trigger reinit */
        return 0;
    }

    rc = mcp2515_send(TYPE_file_read, ID_sd, NULL, 0);

    return rc;
}

uint8_t debug_irq(void)
{
    uint8_t rc;
    char buf[64];
    fgets(buf, sizeof(buf), stdin);
    debug_flush();
    buf[strlen(buf)-1] = '\0';

    if (buf[0] == 'c' && buf[1] == '\0') {
        irq_signal |= IRQ_USER;
    } else if (buf[0] == '?') {
        printf_P(PSTR("state: %d\n"), state);
    } else if (buf[0] == 'x' && buf[1] == '\0') {
        state = STATE_CONNECTED;
        puts_P(PSTR("forced connect\n"));
    } else if (buf[0] == 'd' && buf[1] == '\0') {
        TCPDisconnect();
        state = STATE_CLOSED;
    } else if (buf[0] == 's') {
        rc = TCPSend((uint8_t *)buf + 1, strlen(buf) - 1);
        if (rc)
            printf("send: %d\n", rc);
    } else {
        print(buf);
        uart_putchar('\r');
    }

    _delay_ms(500);
    PROMPT;

    return 0;
}

void tcp_irq(uint8_t *buf, uint8_t len)
{
    /* TODO: need to buffer packets here and reassemble
       when possible */
    uint8_t rc;
    uint8_t i;
    struct mcp2515_packet_t p;

    p.type = buf[0];
    p.id = buf[1];
    p.len = buf[2];

    printf_P(PSTR("got TCP: '%s'\n"), buf);

    if (buf[3] > 8) {
        puts_P(PSTR("tcp len er"));
        buf[3] = 8;
    }

    for (i = 0; i < buf[3]; i++)
        p.data[i] = buf[3 + i];

    rc = mcp2515_send2(&p);
    if (rc)
        puts_P(PSTR("can er"));
}

ISR(USART_RX_vect)
{
    char c;
    static char at_buf[64];
    static uint8_t at_buf_len = 0;

    static uint8_t tcp_rx_buf[64];
    static uint8_t tcp_rx_buf_len = 0;

    static uint8_t tcp_toread = 0;

    c = UDR0;

    /* if we're reading a tcp chunk */
    if (tcp_toread > 0) {
        putchar(c);
        tcp_rx_buf[tcp_rx_buf_len] = c;
        tcp_rx_buf[tcp_rx_buf_len + 1] = '\0';
        tcp_toread--;
        tcp_rx_buf_len++;
        if (tcp_toread == 0) {
            tcp_irq(tcp_rx_buf, tcp_rx_buf_len);
            tcp_rx_buf_len = 0;
        }

        return;
    }

    if (state == STATE_EXPECTING_PROMPT && at_buf[0] == '>') {
        state = STATE_GOT_PROMPT;
        at_buf_len = 0;
        at_buf[0] = '\0';
    } else if (c == '\n') {
        /* ignore empty lines */
        if (at_buf_len < 2) {
            at_buf_len = 0;
            return;
        }

        if (streq_P(at_buf, PSTR("OK"))) {
            flag_ok = 1;
            /* We don't care anymore */
        } else if (streq_P(at_buf, PSTR("ERROR"))) {
            flag_error = 1;
            /* We don't care anymore */
        } else if (streq_P(at_buf, PSTR("RING"))) {
            slow_write("ATH\r", strlen("ATH\r"));
            _delay_ms(1400);
            /* trigger remote connect() */
            irq_signal |= IRQ_USER;
        } else if (streq_P(at_buf, PSTR("STATE: IP INITIAL"))) {
            state = STATE_IP_INITIAL;
        } else if (streq_P(at_buf, PSTR("STATE: IP START"))) {
            state = STATE_IP_START;
        } else if (streq_P(at_buf, PSTR("STATE: IP GPRSACT"))) {
            state = STATE_IP_GPRSACT;
        } else if (streq_P(at_buf, PSTR("STATE: IP STATUS"))) {
            state = STATE_IP_STATUS;
        } else if (streq_P(at_buf, PSTR("STATE: IP PROCESSING"))) {
            state = STATE_IP_PROCESSING;
        } else if (streq_P(at_buf, PSTR("CONNECT OK"))) {
            state = STATE_CONNECTED;
        } else if (streq_P(at_buf, PSTR("CONNECT FAIL"))) {
            state = STATE_ERROR;
        } else if (streq_P(at_buf, PSTR("SEND OK"))) {
            state = STATE_CONNECTED;
        } else if (streq_P(at_buf, PSTR("CLOSE OK"))) {
            state = STATE_CLOSED;
        } else if (streq_P(at_buf, PSTR("CLOSED"))) {
            puts_P(PSTR("closed by peer"));
            state = STATE_CLOSED;
        } else if (streq_P(at_buf, PSTR("NORMAL POWER DOWN"))) {
            state = STATE_POWEROFF;
        } else if (streq_P(at_buf, PSTR("+PDP: DEACT"))) {
            state = STATE_ERROR;
        } else if (strstart(at_buf, "+CME ERROR:")) {
            state = STATE_ERROR;
        } else if (streq_P(at_buf, PSTR("SHUT OK"))) {
            state = STATE_CLOSED;
        }

        at_buf_len = 0;
        at_buf[0] = '\0';
    } else if (c != '\r' && c != '\0') {
        if (at_buf_len < sizeof(at_buf) - 1) {
            at_buf[at_buf_len] = c;
            at_buf[at_buf_len + 1] = '\0';
            at_buf_len++;
        } else {
            putchar('@');
        }
    }
    
    if (c == ':' && strstart(at_buf, "+IPD,")) {
        uint8_t len = atoi(at_buf + strlen("+IPD,"));
        tcp_toread = len;
    }
}

uint8_t periodic_irq(void)
{
    /* send a heartbeat to let modemb know we're alive */
#if 0
    rc = mcp2515_send(TYPE_value_periodic, ID_modemb, buf, 3);
    if (rc != 0) {
        return rc;
    }
#endif

    return 0;
}

uint8_t write_packet(void)
{
    uint8_t i;
    uint8_t rc;
    static uint8_t net_buf[64];
    static uint8_t net_buf_len = 0;

    if (((uint16_t)net_buf_len + packet.len + 1 + 1 + 1)
            >= sizeof(net_buf)) {
        rc = TCPSend(net_buf, net_buf_len);
        if (rc) {
            puts_P(PSTR("snd er"));
            return rc;
        }

        net_buf_len = 0;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        net_buf[net_buf_len++] = packet.type;
        net_buf[net_buf_len++] = packet.id;
        net_buf[net_buf_len++] = packet.len;

        for (i = 0; i < packet.len; i++) {
            net_buf[net_buf_len++] = packet.data[i];
        }
    }

    printf("+%d\n", net_buf_len);

    return 0;
}

uint8_t can_irq(void)
{
    uint8_t rc;

    packet.unread = 0;

    switch (packet.type) {
    default:
        if (state == STATE_CONNECTED) {
            rc = write_packet();
            if (rc) {
                printf("sending failed\n");
            } else {
                rc = mcp2515_send(TYPE_xfer_cts, ID_sd, NULL, 0);
            }
        } else {
            //printf("not connected!\n");
        }
        break;
    }

    return 0;
}

void main(void)
{
    NODE_INIT();

    sei();

    NODE_MAIN();
}
