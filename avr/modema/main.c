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
#include "ring.h"

#define streq_P(a,b) (strcmp_P(a, b) == 0)
#define strstart(a,b) (strncmp_P(a, PSTR(b), strlen(b)) == 0)

#define MODEM_SILENCE_TIMEOUT (30 * 3)

volatile uint16_t canary;

static uint8_t at_tx_buf[64];
volatile uint8_t tcp_toread;

volatile uint32_t modem_alive_time;

volatile uint8_t tcp_ring_array[64];
volatile struct ring_t tcp_ring;

uint8_t can_ring_urgent;
volatile uint8_t can_ring_array[64];
volatile struct ring_t can_ring;



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

void handle_packet(void)
{
    uint8_t i;
    uint8_t bufno;
    uint8_t can_buf[4];
    uint8_t can_buf_len;
    uint8_t rc;

    switch (packet.type) {
    case TYPE_at_0_write ... TYPE_at_7_write:
        bufno = packet.type - TYPE_at_0_write;
        for (i = 0; i < packet.len; i++) {
            at_tx_buf[bufno * 8 + i] = packet.data[i];
        }

        break;

    case TYPE_at_send:
        slow_write(at_tx_buf, strlen((const char *)at_tx_buf));
        slow_write("\r", 1);
        break;

    case TYPE_action_modema_powercycle:
        printf("ACTION_POWERCYCLE\n");
        powercycle_modem();
        break;

    case TYPE_action_modema_connect:
        TRIGGER_USER1_IRQ();
        break;

    case TYPE_action_modema_disconnect:
        TCPDisconnect();
        break;

    case TYPE_set_ip:
        ip[0] = packet.data[0];
        ip[1] = packet.data[1];
        ip[2] = packet.data[2];
        ip[3] = packet.data[3];
        break;

    case TYPE_value_request:
        if (packet.id != MY_ID && packet.id != ID_any) {
            break;
        }
        if (packet.sensor == SENSOR_boot) {
            can_buf[0] = boot_mcusr;
            can_buf_len = 1;
        } else if (packet.sensor == SENSOR_interval) {
            can_buf[0] = periodic_interval >> 8;
            can_buf[1] = periodic_interval;
            can_buf_len = 2;
        } else if (packet.sensor == SENSOR_uptime) {
            can_buf[0] = uptime >> 24;
            can_buf[1] = uptime >> 16;
            can_buf[2] = uptime >> 8;
            can_buf[3] = uptime >> 0;
            can_buf_len = 4;
        } else {
            break;
        }

        rc = mcp2515_send_wait(TYPE_value_explicit, MY_ID, packet.sensor,
                can_buf, can_buf_len);
        (void)rc;

        if (ring_space(&can_ring) >= CAN_HEADER_LEN + can_buf_len) {
            ring_put(&can_ring, TYPE_value_explicit);
            ring_put(&can_ring, MY_ID);
            ring_put(&can_ring, packet.sensor >> 8);
            ring_put(&can_ring, packet.sensor >> 0);
            ring_put(&can_ring, can_buf_len);

            for (i = 0; i < can_buf_len; i++) {
                ring_put(&can_ring, can_buf[i]);
            }

            can_ring_urgent = 1;

            TRIGGER_USER3_IRQ();
        } else {
            printf("DROP %d\n", __LINE__);
        }
        break;
    }
}

uint8_t user1_irq(void)
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

void tcp_enqueue_packet(void)
{
    uint8_t i;

    if (ring_space(&can_ring) < CAN_HEADER_LEN + packet.len) {
        printf("DROP %d\n", __LINE__);
        return;
    }

    if (packet.type != TYPE_value_periodic && packet.type != TYPE_xfer_chunk
            && packet.type != TYPE_xfer_cts) {
        can_ring_urgent = 1;
    }

    ring_put(&can_ring, packet.type);
    ring_put(&can_ring, packet.id);
    ring_put(&can_ring, (packet.sensor >> 8));
    ring_put(&can_ring, (packet.sensor & 0xFF));
    ring_put(&can_ring, packet.len);

    for (i = 0; i < packet.len; i++) {
        ring_put(&can_ring, packet.data[i]);
    }

    TRIGGER_USER3_IRQ();

    printf("ENQ %d %d\n", 5 + packet.len, packet.type);
}


uint8_t debug_irq(void)
{
    char buf[64];
    fgets(buf, sizeof(buf), stdin);
    debug_flush();
    buf[strlen(buf) - 1] = '\0';

    switch (buf[0]) {
    case 'p':
        printf("DEBUG POWERCYCLE\n");
        powercycle_modem();
        break;
    case '1':
        TRIGGER_USER1_IRQ();
        break;
    case '2':
        TRIGGER_USER2_IRQ();
        break;
    case '3':
        TRIGGER_USER3_IRQ();
        break;
    case '4':
        packet.type = TYPE_value_periodic;
        packet.id = ID_arm;
        packet.sensor = SENSOR_voltage;
        packet.len = 5;
        packet.data[0] = 1;
        packet.data[1] = 2;
        packet.data[2] = 3;
        packet.data[3] = 4;
        packet.data[4] = 5;
        tcp_enqueue_packet();
        break;
    case '5':
        packet.type = TYPE_value_explicit;
        packet.id = ID_drive;
        packet.sensor = SENSOR_current;
        packet.len = 6;
        packet.data[0] = 1;
        packet.data[1] = 2;
        packet.data[2] = 3;
        packet.data[3] = 4;
        packet.data[4] = 5;
        packet.data[5] = 6;
        tcp_enqueue_packet();
        break;
    case 'x':
        state = STATE_CONNECTED;
        puts_P(PSTR("conn!\n"));
        break;
    case 'c':
        TRIGGER_USER1_IRQ();
        break;

    case 'd':
        TCPDisconnect();
        state = STATE_CLOSED;
        break;
    case 'z':
        printf("%u\n", stack_space());
        printf("%u\n", free_ram());
        break;
    case '?':
        mcp2515_dump();
        break;
    case 'm':
        mcp2515_init();
        break;
    case 'A':
        print(buf);
        uart_putchar('\r');
        break;
    }

    _delay_ms(100);
    PROMPT;

    return 0;
}

uint8_t user2_irq(void)
{
    uint8_t rc;
    uint8_t i;
    struct mcp2515_packet_t p;

    while (!ring_empty(&tcp_ring)) {
        p.type = ring_get_idx(&tcp_ring, 0);

        if (p.type == TYPE_nop) {
            printf(">NOP\n");
            ring_get(&tcp_ring);
            continue;
        }

        if (ring_bytes(&tcp_ring) < 5) {
            return 0;
        }

        p.len = ring_get_idx(&tcp_ring, 4);
        if (p.len > 8) {
            p.len = 8;
        }

        if (ring_bytes(&tcp_ring) < (1 + 1 + 2 + 1 + p.len)) {
            printf("return 0\n");
            return 0;
        }

        p.type = ring_get(&tcp_ring);
        p.id = ring_get(&tcp_ring);
        p.sensor =
            (ring_get(&tcp_ring) << 8) |
            (ring_get(&tcp_ring) << 0);

        (void)ring_get(&tcp_ring);

        for (i = 0; i < p.len; i++) {
            p.data[i] = ring_get(&tcp_ring);
        }

        rc = mcp2515_send2(&p);

        if (rc) {
            //printf("can er %d\n", rc);
        } else {
            //printf("CAN sent\n");
        }

        packet = p;

        handle_packet();
    }

    return 0;
}

uint8_t user3_irq(void)
{
    uint8_t rc;

    if (state == STATE_GOT_PROMPT) {
        printf("u3: busy\n");
        return 0;
    }

    if (ring_empty(&can_ring)) {
        return 0;
    }

    /* TODO: maybe have a maximum timeout to send stale data */

    if ((ring_bytes(&can_ring) < can_ring.size / 2) && can_ring_urgent == 0) {
        printf("u3: waiting\n");
        return 0;
    }

    rc = TCPSend();

    TRIGGER_USER3_IRQ();

    return rc;
}

ISR(USART_RX_vect)
{
    char c;
    static char at_rx_buf[64];
    static uint8_t at_rx_buf_len = 0;

    c = UDR0;

    modem_alive_time = now;

    /* if we're reading a tcp chunk */
    if (tcp_toread > 0) {
        if (ring_space(&tcp_ring) > 0) {
            ring_put(&tcp_ring, c);
        }
        tcp_toread--;
        if (tcp_toread == 0) {
            TRIGGER_USER2_IRQ();
        }
    } else if (state == STATE_EXPECTING_PROMPT) {
        if (c == '>') {
            state = STATE_GOT_PROMPT;
            at_rx_buf_len = 0;
            at_rx_buf[0] = '\0';
        }
    } else if (c == ':' && strstart(at_rx_buf, "+IPD,")) {
        state = STATE_CONNECTED;

        TRIGGER_USER3_IRQ();

        uint8_t len = atoi(at_rx_buf + strlen("+IPD,"));
        tcp_toread = len;

        at_rx_buf_len = 0;
        at_rx_buf[0] = '\0';
    } else if (c == '\n') {
        /* ignore empty lines */
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
            TRIGGER_USER1_IRQ();
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
            state = STATE_CLOSED;
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
            state = STATE_CLOSED;
        } else if (strstart(at_rx_buf, "+CME ERROR:")) {
            state = STATE_CLOSED;
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

    state = STATE_ERROR;

    slow_write("AT+CIPSTATUS\r", strlen("AT+CIPSTATUS\r"));
    _delay_ms(500);

    if (state == STATE_ERROR) {
        printf("STATE ERROR POWERCYCLE\n");
        powercycle_modem();
    }
}

uint8_t periodic_irq(void)
{
    if (state == STATE_EXPECTING_PROMPT || state == STATE_GOT_PROMPT) {
        printf("prompt\n");
        return 0;
    }

    printf("STATE %d\n", state);

    if (state == STATE_CONNECTED) {
        if (ring_space(&can_ring) > 0) {
            ring_put(&can_ring, TYPE_nop);
            can_ring_urgent = 1;
            /* TODO: only send NOP after inactivity */
            TRIGGER_USER3_IRQ();
        } else {
            printf("DROP %d\n", __LINE__);
        }
    }

    if (now - modem_alive_time >= MODEM_SILENCE_TIMEOUT) {
        tcp_toread = 0;

        check_modem_alive();
    }

    return 0;
}

uint8_t can_irq(void)
{
    uint8_t rc;

    while (mcp2515_get_packet(&packet) == 0) {
        handle_packet();

        if (state == STATE_CONNECTED) {
            tcp_enqueue_packet();

            if (packet.type == TYPE_xfer_chunk || packet.type == TYPE_ircam_header
                    || packet.type == TYPE_laser_sweep_header) {
                printf("CTS:%d\n", packet.id);
                rc = mcp2515_send(TYPE_xfer_cts, packet.id, NULL, 0);
                (void)rc;
            }
        }
    }

    return 0;
}

void sleep(void)
{
    /*
    sleep_enable();
    sleep_bod_disable();
    sei();
    sleep_cpu();
    sleep_disable();
    */
}


int main(void)
{
    NODE_INIT();

    tcp_ring = ring_init(tcp_ring_array, sizeof(tcp_ring_array));
    can_ring = ring_init(can_ring_array, sizeof(can_ring_array));

    sei();

    check_modem_alive();

    NODE_MAIN();
}
