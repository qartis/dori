#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "sim900.h"
#include "uart.h"
#include "can.h"
#include "mcp2515.h"
#include "ring.h"

volatile enum state state;
volatile uint8_t flag_ok;
volatile uint8_t flag_error;
uint8_t ip[4] = { 192, 81, 218, 115 };

uint8_t wait_for_state(uint8_t goal_state)
{
    uint8_t retry;

    /* 255 * 10 = 2550 ms */
    retry = 255;
    while (state != goal_state && state != STATE_ERROR && --retry)
        _delay_ms(10);

    return state != goal_state;
}

uint8_t wait_for_ok(void)
{
    uint8_t retry;

    /* 255 * 30 = 7650 ms */
    retry = 255;
    while (flag_ok == 0 && flag_error == 0 && --retry)
        _delay_ms(30);

    return flag_ok;
}

void slow_write(const void *buf, uint16_t count)
{
    uint16_t i = 0;
    const char *c = (const char *)buf;

    /* delay BEFORE writing, so we can definitely capture the response */
    while (i < count) {
        if (c[i] == '\r') {
            _delay_ms(10);
        } else {
            _delay_ms(1);
        }

        uart_putchar(c[i]);
        i++;
    }
}

void sendATCommand(const char *PROGMEM fmt, ...)
{
    char buf[128];
    char fmt_copy[128];
    va_list ap;

    va_start(ap, fmt);
    strcpy_P(fmt_copy, fmt);
    vsnprintf(buf, sizeof(buf), fmt_copy, ap);
    va_end(ap);

    flag_ok = 0;
    flag_error = 0;

    _delay_ms(10);

    slow_write(buf, strlen(buf));
    slow_write("\r", strlen("\r"));
}

uint8_t TCPDisconnect(void)
{
    state = STATE_CLOSED;

    sendATCommand(PSTR("AT+CIPCLOSE"));
//    wait_for_state(STATE_CLOSED);

    _delay_ms(1000);

    sendATCommand(PSTR("AT+CIPSHUT"));
//    wait_for_state(STATE_CLOSED);

    sendATCommand(PSTR("AT+CIPSTATUS"));
//    rc = wait_for_state(STATE_IP_INITIAL);

    return 0;
}

uint8_t TCPSend(void)
{
    uint8_t rc;
    uint16_t i;
    uint16_t err_num;
    uint8_t count;
    uint8_t c;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        count = ring_bytes(&can_ring);
        can_ring_urgent = 0;
    }

    printf("TCPSend(%d)\n", count);

    state = STATE_EXPECTING_PROMPT;
    sendATCommand(PSTR("AT+CIPSEND=%d"), count);

    rc = wait_for_state(STATE_GOT_PROMPT);
    if (rc != 0) {
        printf("NO PROMPT\n");

        for (i = 0; i < count; i++) {
            uart_putchar(TYPE_nop);
            _delay_us(500);
        }

        err_num = __LINE__;
        mcp2515_send_wait(TYPE_sensor_error, MY_ID, 0, &err_num,
                sizeof(err_num));

        state = STATE_CLOSED;

        return 1;
    }

    for (i = 0; i < count; i++) {
        c = ring_get(&can_ring);
        uart_putchar(c);
        _delay_us(500);
    }

    rc = wait_for_state(STATE_CONNECTED);
    if (rc != 0) {
        printf("send err %d\n", rc);
    } else {
        printf("send ok\n");
    }

    return rc;
}

uint8_t TCPConnect(void)
{
    uint8_t rc;
    uint8_t retry;

    TCPDisconnect();

    /* Turn on header for received data */
    sendATCommand(PSTR("AT+CIPMUX=0"));

    _delay_ms(10);

    sendATCommand(PSTR("AT+CIPHEAD=1"));

    sendATCommand(PSTR("AT+CIPSTATUS"));
    rc = wait_for_state(STATE_IP_INITIAL);
    if (rc != 0) {
        puts_P(PSTR("err: INITIAL"));
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    puts_P(PSTR("state: IP INITIAL"));

    sendATCommand(PSTR("AT+CSTT=\"goam.com\",\"wapuser1\",\"wap\""));

    _delay_ms(100);

    retry = 3;
    do {
        sendATCommand(PSTR("AT+CIPSTATUS"));
        rc = wait_for_state(STATE_IP_START);
    } while (rc != 0 && --retry);

    if (state != STATE_IP_START) {
        puts_P(PSTR("err: START"));
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    puts_P(PSTR("state: IP START"));

    /* This command takes a little while */
    flag_ok = 0;
    sendATCommand(PSTR("AT+CIICR"));
    wait_for_ok();

    _delay_ms(100);
    sendATCommand(PSTR("AT+CIPSTATUS"));
    rc = wait_for_state(STATE_IP_GPRSACT);
    if (rc != 0) {
        _delay_ms(500);
        sendATCommand(PSTR("AT+CIPSTATUS"));
        rc = wait_for_state(STATE_IP_GPRSACT);
        if (rc != 0) {
            puts_P(PSTR("err: GPRSACT"));
            /* dispatch a MODEM_ERROR CAN packet and try to continue */
            return rc;
        }
    }

    puts_P(PSTR("state: IP GPRSACT"));

    /* attach to GPRS network, display IP address */
    sendATCommand(PSTR("AT+CIFSR"));
    _delay_ms(1500);

    sendATCommand(PSTR("AT+CIPSTATUS"));
    rc = wait_for_state(STATE_IP_STATUS);
    if (rc != 0) {
        puts_P(PSTR("err: STATUS"));
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    puts_P(PSTR("state: IP STATUS"));

    sendATCommand(PSTR("AT+CIPSTART=\"TCP\",\"%d.%d.%d.%d\",\"53\""), ip[0],
                  ip[1], ip[2], ip[3]);

    rc = wait_for_state(STATE_CONNECTED);
    if (rc != 0) {
        sendATCommand(PSTR("AT+CIPSTATUS"));
        rc = wait_for_state(STATE_CONNECTED);
    }

    if (rc != 0) {
        if (state == STATE_IP_PROCESSING) {
            puts_P(PSTR("conn: timeout"));
            /* don't cancel the attempt. it might succeed later */
        } else {
            puts_P(PSTR("conn: refused"));
        }

        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    puts_P(PSTR("state: CONNECTED"));

    return 0;
}
