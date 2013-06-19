#include <avr/io.h>
#include <stdlib.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>

#include "uart.h"
#include "debug.h"
#include "modem.h"

volatile struct {
    uint8_t received : 1;
    uint8_t ok : 1;
    uint8_t error : 1;
    uint8_t nocarrier : 1;
    uint8_t connect : 1;
    uint8_t ack : 1;
} flags;

volatile uint32_t ack_len;

volatile enum state {
    STATE_AT,
    STATE_ACK,
    STATE_LEN,
    STATE_TYPE,
    STATE_PAYLOAD,
    STATE_DONE
} state;

volatile uint8_t modem_buf[512];
volatile uint16_t modem_len;
volatile uint8_t modem_packet_type;

static void parse_at(volatile const uint8_t *buf)
{
    if (buf[0] == 'O' && buf[1] == 'K') {
        flags.ok = 1;
    } else if (buf[0] == 'E' && buf[1] == 'R') {
        flags.error = 1;
    } else if (buf[0] == 'N' && buf[1] == 'O') {
        flags.nocarrier = 1;
    } else if (buf[0] == 'C' && buf[1] == 'O') {
        flags.connect = 1;
    } else if (buf[0]) {
        /* unknown at command */
    }
}


ISR(USART_RX_vect)
{
    uint8_t c;
    static uint32_t len = 0;
    static uint16_t received = 0;

    c = UDR0;

    flags.received = 1;

    switch (state) {
    case STATE_AT:
        if (c == '\r') {
            modem_buf[received] = '\0';
            parse_at(modem_buf);
            received = 0;
        } else if (c != '\n' && c != '\0') {
            modem_buf[received++] = c;
        }
        break;
    case STATE_ACK:
        debugf("got byte %c\n", c);
        len <<= 7;
        len |= c & 0x7f;
        if ((c & 0x80) == 0) {
            state = STATE_LEN;
            ack_len = len;
            len = 0;
        }
        break;
    case STATE_LEN:
        len <<= 7;
        len |= c & 0x7f;
        if ((c & 0x80) == 0) {
            state = STATE_TYPE;
        }
        break;
    case STATE_TYPE:
        modem_packet_type = c;
        state = STATE_PAYLOAD;
        received = 0;
        break;
    case STATE_PAYLOAD:
        modem_buf[received] = c;
        received++;
        if (received == len) {
            modem_len = received;
            state = STATE_DONE;
        }
        break;
    case STATE_DONE:
        /* ignore trailing bytes */
        break;
    }
}

void slow_write(const char *buf)
{
    uint8_t i;

    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        flags.ok = 0;
        flags.error = 0;
        flags.nocarrier = 0;
        flags.connect = 0;
        flags.received = 0;
    }

    for (i = 0; i < strlen(buf); i++) {
        uart_putchar(buf[i]);
        _delay_us(1000);
    }
}

uint8_t send_packet(uint8_t type, uint8_t *buf, uint32_t len)
{
    uint8_t rc;
    uint8_t retry;
    uint32_t tosend;
    uint32_t ack_len_copy;
    enum state state_copy;
    
    rc = connect();
    if (rc) {
        debugf("failed to connect: %d\n", rc);
        return rc;
    }

    /*

    if (len < (1 << 7)) goto one;
    else if (len < (1 << 14)) goto two;
    else if (len < ((uint32_t)1 << 21)) goto three;
    else goto four;
four:
    uart_putchar((1 << 7) | ((len >> 21) & 0xff));
three:
    uart_putchar((1 << 7) | ((len >> 14) & 0xff));
two:
    uart_putchar((1 << 7) | ((len >> 7) & 0xff));
one:
    uart_putchar((0 << 7) | ((len >> 0) & 0x7f));
    */
    debug("put len 1\n");

    flags.received = 0;

    slow_write("temp:\n");


    if (type != TYPE_PING)
        uart_putchar(type);

    tosend = len;
    while (tosend--) {
     //   if (flags.received) break;
        uart_putchar(*buf);
        debugf("sent byte %x\n", *buf);
        buf++;
    }

#if 0
    if (len > 0 && tosend != 0) {
        /* transfer was aborted. immediately ping */
        debug("xfer aborted\n");
        disconnect();
        _delay_ms(3000);
        return send_packet(TYPE_PING, NULL, 0);
    }
#endif

    /* wait until we receive tk's ack */
    retry = 255;
    do {
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            ack_len_copy = ack_len;
            state_copy = state;
        }
        _delay_ms(50);
    } while (state_copy == STATE_ACK && --retry);

    if (retry == 0) {
        debug("no ack?\n");
        /* tk didn't ack our transfer */
        goto abort;
    }

    if (ack_len_copy != len) {
        debug("bad ack?\n");
        /* tk sent a wrong ack size? abort their transfer */
        goto abort;
    }

    /* wait until we receive tk's entire payload */
    retry = 255;
    do {
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            state_copy = state;
        }
        _delay_ms(100);
    } while (state_copy != STATE_DONE && --retry);

    if (retry == 0){
        debug("no payload?\n");
        goto abort;
    }

    return 0;

abort:
    uart_putchar('\0');
    _delay_ms(500);
    disconnect();
    return 4;
}

void find_modem(void)
{
    uint16_t delay = 1000;

    debug("looking for modem..");

    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        state = STATE_AT;
    }

    for (;;) {
        debug(".");

        slow_write("ATE0\r");
        _delay_ms(100);

        if (flags.ok)
            break;

        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            _delay_ms(1100);
            slow_write("+++");
            _delay_ms(delay);
        }

        if (delay < 60000)
            delay += 1000;
    }

    slow_write("AT+CGDCONT=1,\"IP\",\"goam.com\",,\r");
    _delay_ms(10);

    slow_write("AT%CGPCO=1,\"PAP,wapuser1,wap\",2\r");
    _delay_ms(10);

    slow_write("AT$DESTINFO=\"h.qartis.com\",1,53,1\r");
    _delay_ms(100);

    debug("found!\n");
}

uint8_t connect(void)
{
    uint8_t retry;

    debug("connecting\n");

    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        state = STATE_AT;
    }

    slow_write("At\r");
    _delay_ms(10);

    slow_write("aT\r");
    _delay_ms(500);

    if (!flags.ok)
        return 1;

    slow_write("ATD*97#\r");
    _delay_ms(750);

    retry = 255;
    while (!flags.ok && --retry)
        _delay_ms(100);

    if (!flags.ok)
        return 2;

    _delay_ms(300);

    if (!flags.ok || flags.error || flags.nocarrier || flags.connect){
        debug("flags!\n");
        return 3;
    }

    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        ack_len = 0;
        state = STATE_ACK;
    }

    return 0;
}

uint8_t disconnect(void)
{
    _delay_ms(1500);
    slow_write("+++");
    _delay_ms(1500);

    _delay_ms(1500);
    slow_write("+++");
    _delay_ms(1500);

    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        state = STATE_AT;
    }

    return 0;
}
