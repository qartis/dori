#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "sim900.h"
#include "uart.h"
#include "debug.h"

volatile enum state state;
volatile uint8_t flag_ok;
volatile uint8_t flag_error;

uint8_t wait_for_state(uint8_t goal_state)
{
    uint8_t retry;

    /* 255 * 39 = 10000 ms */
    retry = 255;
    while (state != goal_state && state != STATE_ERROR && --retry)
        _delay_ms(39);

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

    while (i < count) {
        uart_putchar(c[i]);
        _delay_ms(1);

        if (c[i] == '\r')
            _delay_ms(10);

        i++;
    }
}

void sendATCommand(const char * PROGMEM fmt, ...)
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

    slow_write(buf, strlen(buf));
    slow_write("\r", strlen("\r"));

    _delay_ms(100);
}

uint8_t TCPDisconnect(void)
{
    uint8_t rc;

    state = STATE_UNKNOWN;

    sendATCommand(PSTR("AT+CIPCLOSE"));
//    wait_for_state(STATE_CLOSED);

    state = STATE_UNKNOWN;

    sendATCommand(PSTR("AT+CIPSHUT"));
//    wait_for_state(STATE_CLOSED);

    sendATCommand(PSTR("AT+CIPSTATUS"));
//    rc = wait_for_state(STATE_IP_INITIAL);

    return 0;
}

uint8_t TCPSend(uint8_t *buf, uint16_t count)
{
    uint8_t rc;
    uint16_t i;

    state = STATE_EXPECTING_PROMPT;
    sendATCommand(PSTR("AT+CIPSEND=%d"), count);

    rc = wait_for_state(STATE_GOT_PROMPT);
    if (rc != 0) {
        printf_P(PSTR("wanted to send data, but didn't get prompt (state now %d)\n"), state);
        /* send CAN error, but continue regardless */

        return 1;
    }
  
    for (i = 0; i < count; i++) {
        uart_putchar(buf[i]);
    }

    rc = wait_for_state(STATE_CONNECTED);
    rc = wait_for_state(STATE_CONNECTED);
    if (rc != 0) {
        printf_P(PSTR("Sending failed\n"));
    } else {
        printf_P(PSTR("Sending succeeded\n"));
    }

    return rc;
}

uint8_t TCPConnect(void)
{
    uint8_t rc;

    TCPDisconnect();

    /* Turn on header for received data */
    sendATCommand(PSTR("AT+CIPMUX=0"));
    sendATCommand(PSTR("AT+CIPHEAD=1"));

    sendATCommand(PSTR("AT+CIPSTATUS"));
    rc = wait_for_state(STATE_IP_INITIAL);
    if (rc != 0) {
        printf_P(PSTR("failed to get IP state INITIAL\n"));
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf_P(PSTR("state: IP INITIAL\n"));

    sendATCommand(PSTR("AT+CSTT=\"goam.com\",\"wapuser1\",\"wap\""));

    sendATCommand(PSTR("AT+CIPSTATUS"));
    rc = wait_for_state(STATE_IP_START);
    if (rc != 0) {
        printf_P(PSTR("failed to get IP state START\n"));
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf_P(PSTR("state: IP START\n"));

    /* This command takes a little while */
    sendATCommand(PSTR("AT+CIICR"));
    //_delay_ms(2000);

    _delay_ms(500);
    sendATCommand(PSTR("AT+CIPSTATUS"));
    _delay_ms(500);
    sendATCommand(PSTR("AT+CIPSTATUS"));
    wait_for_state(STATE_IP_GPRSACT);
    rc = wait_for_state(STATE_IP_GPRSACT);
    rc = wait_for_state(STATE_IP_GPRSACT);
    if (rc != 0) {
        printf_P(PSTR("Failed to get IP state GPRSACT\n"));
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf_P(PSTR("state: IP GPRSACT\n"));

    /* attach to GPRS network, display IP address */
    sendATCommand(PSTR("AT+CIFSR"));
    wait_for_ok();

    sendATCommand(PSTR("AT+CIPSTATUS"));
    rc = wait_for_state(STATE_IP_STATUS);
    if (rc != 0) {
        printf_P(PSTR("Failed to get IP state STATUS\n"));
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf_P(PSTR("state: IP STATUS\n"));

    sendATCommand(PSTR("AT+CIPSTART=\"TCP\",\"69.172.149.225\",\"53\""));

    sendATCommand(PSTR("AT+CIPSTATUS"));
    _delay_ms(500);
    sendATCommand(PSTR("AT+CIPSTATUS"));
    rc = wait_for_state(STATE_CONNECTED);
    if (rc != 0) {
        if (state == STATE_IP_PROCESSING) {
            printf_P(PSTR("connect: timeout\n"));
            /* don't cancel the attempt. it might succeed later */
        } else {
            printf_P(PSTR("connect: connection refused\n"));
        }

        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf_P(PSTR("state: CONNECTED\n"));

    return 0; 
}
