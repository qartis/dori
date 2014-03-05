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

#define MODEM_SILENCE_TIMEOUT 30

static uint8_t at_tx_buf[64];
static uint8_t tcp_toread;

volatile uint32_t modem_alive_time;

uint8_t uart_irq(void)
{
    return 0;
}

void powercycle_modem(void)
{
    printf("pwrcycl\n");
    DDRD |= (1 << PORTD2);
    PORTD |= (1 << PORTD2);

    _delay_ms(4000);

    PORTD &= ~(1 << PORTD2);
    DDRD &= ~(1 << PORTD2);

    _delay_ms(1000);

    slow_write("AT\r", strlen("AT\r"));
    _delay_ms(100);
    slow_write("AT\r", strlen("AT\r"));
    _delay_ms(100);
}

uint8_t user_irq(void)
{
    uint8_t rc;

    printf("user irq\n");

    if ((now - modem_alive_time < MODEM_SILENCE_TIMEOUT) && 
            state == STATE_CONNECTED) {
        printf("still connected\n");
        return 0;
    }

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

    if (buf[1] == '\0') {
        switch (buf[0]) {
        case 'p':
            powercycle_modem();
            break;
        case 'c':
            TRIGGER_USER_IRQ();
            break;
        case '?':
            printf_P(PSTR("state: %d\n"), state);
            break;
        case 'x':
            state = STATE_CONNECTED;
            puts_P(PSTR("conn!\n"));
            break;
        case 'd':
            TCPDisconnect();
            state = STATE_CLOSED;
            break;
        case 'z':
            printf("%u\n", stack_space());
            printf("%u\n", free_ram());
            break;
        }
    } else if (buf[0] == 's') {
        rc = TCPSend((uint8_t *)buf + 1, strlen(buf) - 1);
        printf("send: %d\n", rc);
    } else if(buf[0] == 'z') {
        printf("%u\n", stack_space());
        printf("%u\n", free_ram());
    } else {
        print(buf);
        uart_putchar('\r');
    }

    _delay_ms(100);
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

    for (i = 0; i < len; i++) {
        printf("%d: %02x\n", i, buf[i]);
        tcp_buf[tcp_buf_len] = buf[i];
        tcp_buf_len++;
        if (tcp_buf_len >= sizeof(tcp_buf)) {
            printf("WTF WTF WTF\n");
        }
    }

    while (tcp_buf_len > 0) {
        p.type = tcp_buf[0];

        if (p.type == TYPE_nop) {
            printf("skipping nop\n");
            tcp_buf_len -= 1;
            memmove(tcp_buf, tcp_buf + 1, tcp_buf_len);
            continue;
        }

        if (tcp_buf_len < 5)
            return;

        if (tcp_buf[4] > 8)
            tcp_buf[4] = 8;

        if (tcp_buf_len < (1 + 1 + 2 + 1 + tcp_buf[4]))
            return;

        p.type = tcp_buf[0];
        p.id = tcp_buf[1];
        p.sensor = (tcp_buf[2] << 8) | tcp_buf[3];
        p.len = tcp_buf[4];

        for (i = 0; i < tcp_buf[4]; i++)
            p.data[i] = tcp_buf[5 + i];

        tcp_buf_len -= 5 + p.len;
        memmove(tcp_buf, tcp_buf + 5 + p.len, tcp_buf_len);

        rc = mcp2515_send2(&p);
        if (rc)
            printf_P(PSTR("can er %d\n"), rc);
        else
            puts_P(PSTR("CAN sent"));


        switch (p.type) {
        case TYPE_action_modema_powercycle:
            powercycle_modem();
            break;

        case TYPE_action_modema_connect:
            TRIGGER_USER_IRQ();
            break;

        case TYPE_action_modema_disconnect:
            TCPDisconnect();
            break;
        }
    }
}

ISR(USART_RX_vect)
{
    char c;
    static char at_rx_buf[64];
    static uint8_t at_rx_buf_len = 0;

    static uint8_t tcp_rx_buf[64];
    static uint8_t tcp_rx_buf_len = 0;


    c = UDR0;

    modem_alive_time = now;

    /* if we're reading a tcp chunk */
    if (tcp_toread > 0) {
        tcp_rx_buf[tcp_rx_buf_len] = c;
        tcp_rx_buf[tcp_rx_buf_len + 1] = '\0';
        tcp_toread--;
        tcp_rx_buf_len++;
        if (tcp_toread == 0) {
            tcp_irq(tcp_rx_buf, tcp_rx_buf_len);
            tcp_rx_buf_len = 0;
        }
    } else if (state == STATE_EXPECTING_PROMPT && at_rx_buf[0] == '>') {
        state = STATE_GOT_PROMPT;
        at_rx_buf_len = 0;
        at_rx_buf[0] = '\0';
    } else if (c == ':' && strstart(at_rx_buf, "+IPD,")) {
        // If we got some data, we must be connected
        state = STATE_CONNECTED;

        uint8_t len = atoi(at_rx_buf + strlen("+IPD,"));
        tcp_toread = len;

        at_rx_buf_len = 0;
        at_rx_buf[0] = '\0';
    } else if (c == '\n') {
        /* ignore empty lines */
        /* BE CAREFUL WITH THIS NUMBER */
        if (at_rx_buf_len < 2) {
            at_rx_buf_len = 0;
            at_rx_buf[0] = '\0';
            return;
        }

        if (streq_P(at_rx_buf, PSTR("OK"))) {
            flag_ok = 1;
        } else if (streq_P(at_rx_buf, PSTR("ERROR"))) {
            flag_error = 1;
        } else if (streq_P(at_rx_buf, PSTR("RING"))) {
            printf("ring\n");
            TRIGGER_USER_IRQ();
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
        } else if (streq_P(at_rx_buf, PSTR("CONNECT OK"))) {
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
            state = STATE_ERROR;
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
        } else if (state == STATE_GOT_PROMPT) {
        } else {
            putchar('@');
        }
    }
}

void check_modem_alive(void)
{
    /* send <ESC> to cancel any pending tcp SEND prompts */
    slow_write("\x1b", 1);
    _delay_ms(50);

    slow_write("AT\r", strlen("AT\r"));
    _delay_ms(50);

    slow_write("ATE0\r", strlen("ATE0\r"));
    _delay_ms(50);

    slow_write("AT+CIPSTATUS\r", strlen("AT+CIPSTATUS\r"));
    _delay_ms(500);

    if (now - modem_alive_time >= MODEM_SILENCE_TIMEOUT) {
        powercycle_modem();
    } else {
        //            printf("j5 is alive\n");
    }
}

uint8_t periodic_irq(void)
{
    if (state == STATE_EXPECTING_PROMPT ||
       state == STATE_GOT_PROMPT) {
        printf("expecting prompt\n");
        return 0;
    }

    if (now - modem_alive_time >= MODEM_SILENCE_TIMEOUT) {
        printf("periodic_irq\n");
        if (tcp_toread > 0) {
            tcp_toread = 0;
        }

        check_modem_alive();

        if (state == STATE_CONNECTED) {
            uint8_t type = TYPE_nop;
            TCPSend(&type, sizeof(type));
            printf("sent nop\n");
        }
    }

    return 0;
}

uint8_t write_packet(void)
{
    uint8_t i;
    uint8_t rc;
    /* this should be 256 when we launch */
    static uint8_t net_buf[32];
    static uint16_t net_buf_len;

    if (((uint16_t)net_buf_len + 5 + packet.len)
            >= sizeof(net_buf)) {
        rc = TCPSend(net_buf, net_buf_len);
        if (rc) {
            printf_P(PSTR("snd er %d\n"), rc);
            return rc;
        }

        net_buf_len = 0;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        net_buf[net_buf_len++] = packet.type;
        net_buf[net_buf_len++] = packet.id;
        net_buf[net_buf_len++] = (packet.sensor >> 8);
        net_buf[net_buf_len++] = (packet.sensor & 0xFF);
        net_buf[net_buf_len++] = packet.len;

        for (i = 0; i < packet.len; i++) {
            net_buf[net_buf_len++] = packet.data[i];
        }
    }

    printf_P(PSTR("+%u\n"), net_buf_len);

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

    case TYPE_action_modema_powercycle:
        powercycle_modem();
        break;

    case TYPE_action_modema_connect:
        TRIGGER_USER_IRQ();
        break;

    case TYPE_action_modema_disconnect:
        TCPDisconnect();
        break;

    default:
        if (state == STATE_CONNECTED) {
            //if (packet.type == TYPE_value_periodic) break;
            rc = write_packet();
            if (rc) {
                puts_P(PSTR("sending failed"));
            } else if (packet.type == TYPE_xfer_chunk ||
                       packet.type == TYPE_ircam_header) {
                printf("sending cts\n");
                rc = mcp2515_send(TYPE_xfer_cts, ID_cam, NULL, 0);
            }
        }
        break;
    }

    packet.unread = 0;

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

    //PRR |= (1 << PRTWI) | (1 << PRADC);

    sei();

    check_modem_alive();

    NODE_MAIN();
}
