#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
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
#include "free_ram.h"

#define streq_P(a,b) (strcmp_P(a, b) == 0)
#define strstart(a,b) (strncmp_P(a, PSTR(b), strlen(b)) == 0)

static uint8_t at_tx_buf[64];

static uint8_t tmp_buf[64];
static uint8_t tmp_buflen;

uint8_t uart_irq(void)
{
    return 0;
}

uint8_t user_irq(void)
{
    uint8_t rc;

    if (state == STATE_CONNECTED) return 0;

    slow_write("AT\r", strlen("AT\r"));
    _delay_ms(100);
    slow_write("ATH\r", strlen("ATH\r"));

    _delay_ms(500);
    puts_P(PSTR("conn"));
    rc = TCPConnect();
    if (rc) {
        printf("conn: %d\n", rc);

        /* don't trigger reinit */
        return 0;
    }


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
        puts_P(PSTR("conn!\n"));
    } else if (buf[0] == 'd' && buf[1] == '\0') {
        TCPDisconnect();
        state = STATE_CLOSED;
    } else if (buf[0] == 's') {
        rc = TCPSend((uint8_t *)buf + 1, strlen(buf) - 1);
        if (rc)
            printf("send: %d\n", rc);
    } else if(buf[0] == 'z') {
        printf("%u\n", stack_space());
        printf("%u\n", free_ram());
    } else if(buf[0] == 't') {
        uint8_t i;
        printf("printing %d bytes\n", tmp_buflen);
        for(i = 0; i < tmp_buflen; i++) {
            printf("%d: %02x (%c)\n", i, tmp_buf[i], tmp_buf[i]);
        }
        tmp_buflen = 0;
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
    uint8_t rc;
    uint8_t i;
    struct mcp2515_packet_t p;
    static uint8_t tcp_buf[32];
    static uint8_t tcp_buf_len = 0;

    if (buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
        len--;
    }

    //printf_P(PSTR("got TCP: '%s'\n"), buf);

    for (i = 0; i < len; i++) {
        printf("%d: %02x ", i, buf[i]);
        tcp_buf[tcp_buf_len] = buf[i];
        tcp_buf_len++;
    }
    printf("\n");
    switch (tcp_buf[0]) {
    case '1' ... '9':
        len = tcp_buf[0] - '0';

        if (tcp_buf_len <= len)
            return;

        p.type = TYPE_file_read;
        p.id = ID_logger;
        p.len = len;
        for (i = 0; i < len; i++)
            p.data[i] = tcp_buf[i + 1];

        tcp_buf_len -= len + 1;

        memmove(tcp_buf, tcp_buf + len + 1, tcp_buf_len);


        break;
    
    default:
        if (tcp_buf_len < 3)
            return;

        if (tcp_buf_len < (1 + 1 + 1 + tcp_buf[2]))
            return;

        if (tcp_buf[2] > 8)
            tcp_buf[2] = 8;

        p.type = tcp_buf[0];
        p.id = tcp_buf[1];
        p.len = tcp_buf[2];

        for (i = 0; i < tcp_buf[2]; i++)
            p.data[i] = tcp_buf[3 + i];

        tcp_buf_len -= 3 + p.len;

        memmove(tcp_buf, tcp_buf + 3 + p.len, tcp_buf_len);


        break;
    }

    rc = mcp2515_send2(&p);
    if (rc)
        puts_P(PSTR("can er"));
}

ISR(PCINT2_vect)
{
    // ringing stopped
    if(PIND & (1 << PD4))
        return;

    uint8_t i;
    for(i = 0; i < 10; i++) {
        _delay_ms(100);

        if(PIND & (1 << PD4))
            return;
    }

    irq_signal |= IRQ_USER;
}

ISR(USART_RX_vect)
{
    char c;
    static char at_rx_buf[64];
    static uint8_t at_rx_buf_len = 0;

    static uint8_t tcp_rx_buf[64];
    static uint8_t tcp_rx_buf_len = 0;

    static uint8_t tcp_toread = 0;

    c = UDR0;

    if (tmp_buflen < 50)
        tmp_buf[tmp_buflen++] = c;

    //putchar(c);
    /* if we're reading a tcp chunk */
    if (0 && tcp_toread > 0) {
        printf("\n\n\n#########\n\n\n");
        //putchar(c);
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

    if (state == STATE_EXPECTING_PROMPT && at_rx_buf[0] == '>') {
        state = STATE_GOT_PROMPT;
        at_rx_buf_len = 0;
        at_rx_buf[0] = '\0';
    } else if (c == '\n') {
        /* ignore empty lines */
        if (at_rx_buf_len < 5) {
            at_rx_buf_len = 0;
            return;
        }

        puts("got ");
        puts(at_rx_buf);

        if (streq_P(at_rx_buf, PSTR("OK"))) {
            flag_ok = 1;
            /* We don't care anymore */
        } else if (streq_P(at_rx_buf, PSTR("ERROR"))) {
            flag_error = 1;
            /* We don't care anymore */
        } else if (streq_P(at_rx_buf, PSTR("RING"))) {
            /* trigger remote connect() */
            irq_signal |= IRQ_USER;
        } else if (streq_P(at_rx_buf, PSTR("STATE: IP INITIAL"))) {
            state = STATE_IP_INITIAL;
        } else if (streq_P(at_rx_buf, PSTR("STATE: IP START"))) {
            state = STATE_IP_START;
        } else if (streq_P(at_rx_buf, PSTR("STATE: IP GPRSACT"))) {
            state = STATE_IP_GPRSACT;
        } else if (streq_P(at_rx_buf, PSTR("STATE: IP STATUS"))) {
            state = STATE_IP_STATUS;
        } else if (streq_P(at_rx_buf, PSTR("STATE: IP PROCESSING"))) {
            state = STATE_IP_PROCESSING;
        } else if (streq_P(at_rx_buf, PSTR("STATE: CONNECT OK"))) {
            state = STATE_CONNECTED;
        } else if (streq_P(at_rx_buf, PSTR("CONNECT FAIL"))) {
            state = STATE_ERROR;
        } else if (streq_P(at_rx_buf, PSTR("SEND OK"))) {
            state = STATE_CONNECTED;
        } else if (streq_P(at_rx_buf, PSTR("CLOSE OK"))) {
            state = STATE_CLOSED;
        } else if (streq_P(at_rx_buf, PSTR("CLOSED"))) {
            puts_P(PSTR("closed by peer"));
            state = STATE_CLOSED;
        } else if (streq_P(at_rx_buf, PSTR("NORMAL POWER DOWN"))) {
            state = STATE_POWEROFF;
        } else if (streq_P(at_rx_buf, PSTR("+PDP: DEACT"))) {
            state = STATE_ERROR;
        } else if (strstart(at_rx_buf, "+CME ERROR:")) {
            state = STATE_ERROR;
        } else if (streq_P(at_rx_buf, PSTR("SHUT OK"))) {
            state = STATE_CLOSED;
        }

        at_rx_buf_len = 0;
        at_rx_buf[0] = '\0';
    } else if (c != '\r' && c != '\0') {
        if (at_rx_buf_len < sizeof(at_rx_buf) - 1) {
            at_rx_buf[at_rx_buf_len] = c;
            at_rx_buf[at_rx_buf_len + 1] = '\0';
            at_rx_buf_len++;
        } else {
            putchar('@');
        }
    }

    if (c == ':' && strstart(at_rx_buf, "+IPD,")) {
        uint8_t len = atoi(at_rx_buf + strlen("+IPD,"));
        uint8_t i;
        for(i = 0; i < at_rx_buf_len; i++) {
            printf("%d: %c %02x\n", i, at_rx_buf[i], at_rx_buf[i]);
        }
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

    net_buf[0] = packet.type;
    net_buf[1] = packet.id;
    net_buf[2] = packet.len;

    for (i = 0; i < packet.len; i++)
        net_buf[3 + i] = packet.data[i];

    rc = TCPSend(net_buf, packet.len + 3);
    if (rc) {
        puts_P(PSTR("snd er"));
        return rc;
    }

    return 0;

    if (((uint16_t)net_buf_len + 1 + 1 + 1 + packet.len)
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

    //printf_P(PSTR("+%u\n"), net_buf_len);

    return 0;
}

uint8_t can_irq(void)
{
    uint8_t rc;
    uint8_t bufno;
    uint8_t i;
    uint8_t *ptr;

    switch (packet.type) {
    case TYPE_at_0_write ... TYPE_at_7_write:
        bufno = packet.type - TYPE_at_0_write;
        for (i = 0; i < packet.len; i++)
            at_tx_buf[bufno * 8 + i] = packet.data[i];

        break;

    case TYPE_at_send:
        slow_write(at_tx_buf, strlen((const char *)at_tx_buf));
        slow_write("\r", 1);
        break;

    case TYPE_file_offer:
        ptr = (uint8_t *)&packet.data;
        rc = mcp2515_send(TYPE_file_read, packet.id, ptr, packet.len);
        break;

    default:
        if (state == STATE_CONNECTED) {
            if (packet.type == TYPE_value_periodic) break;
            rc = write_packet();
            if (rc) {
                puts_P(PSTR("sending failed"));
            } else {
                rc = mcp2515_send(TYPE_xfer_cts, ID_logger, NULL, 0);
            }
        }
        break;
    }

    packet.unread = 0;

    return 0;
}

void main(void)
{
    NODE_INIT();

    /*
    PCMSK2 |= (1 << PCINT20);
    PCICR |= (1 << PCIE2);
    */

    sei();

    NODE_MAIN();
}
