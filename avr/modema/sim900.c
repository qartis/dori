#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

    /* 255 * 10 = 2550 ms */
    retry = 255;
    while (state != goal_state && state != STATE_ERROR && --retry)
        _delay_ms(20);

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


void slow_write(const char *buf, uint16_t count)
{
    uint16_t i = 0;

    while (i < count) {
        uart_putchar(buf[i]);
        _delay_ms(1);

        if (buf[i] == '\r')
            _delay_ms(10);

        i++;
    }
}

void sendATCommand(const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    flag_ok = 0;
    flag_error = 0;

    slow_write(buf, strlen(buf));
    slow_write("\r", strlen("\r"));

    _delay_ms(400);
}

uint8_t TCPDisconnect(void)
{
    uint8_t rc;

    state = STATE_UNKNOWN;

    sendATCommand("AT+CIPCLOSE=0");
    wait_for_state(STATE_CLOSED);

    state = STATE_UNKNOWN;

    sendATCommand("AT+CIPSHUT");
    wait_for_state(STATE_CLOSED);

    sendATCommand("AT+CIPSTATUS");
    rc = wait_for_state(STATE_IP_INITIAL);

    return rc;
}

uint8_t TCPSend(uint8_t *buf, uint16_t count)
{
    uint8_t rc;
    uint16_t i;

    state = STATE_EXPECTING_PROMPT;
    sendATCommand("AT+CIPSEND=0,%d", count);

    rc = wait_for_state(STATE_GOT_PROMPT);
    if (rc != 0) {
        printf("wanted to send data, but didn't get prompt (state now %d)\n", state);
        /* send CAN error, but continue regardless */

        return 1;
    }
  
    for (i = 0; i < count; i++) {
        uart_putchar(buf[i]);
    }

    rc = wait_for_state(STATE_CONNECTED);
    if (rc != 0) {
        printf("Sending failed\n");
    } else {
        printf("Sending succeeded\n");
    }

    return rc;
}

uint8_t TCPConnect(void)
{
    uint8_t rc;

    TCPDisconnect();

    /* Multi-connection mode in order to encapsulate received data */
    sendATCommand("AT+CIPMUX=1");

    sendATCommand("AT+CIPSTATUS");
    rc = wait_for_state(STATE_IP_INITIAL);
    if (rc != 0) {
        printf("failed to get IP state INITIAL\n");
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf("state: IP INITIAL\n");

    sendATCommand("AT+CSTT=\"goam.com\",\"wapuser1\",\"wap\"");

    sendATCommand("AT+CIPSTATUS");
    rc = wait_for_state(STATE_IP_START);
    if (rc != 0) {
        printf("failed to get IP state START\n");
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf("state: IP START\n");

    /* This command takes a little while */
    sendATCommand("AT+CIICR");
    _delay_ms(2000);

    sendATCommand("AT+CIPSTATUS");
    wait_for_state(STATE_IP_GPRSACT);
    rc = wait_for_state(STATE_IP_GPRSACT);
    if (rc != 0) {
        printf("Failed to get IP state GPRSACT\n");
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf("state: IP GPRSACT\n");

    /* attach to GPRS network, display IP address */
    sendATCommand("AT+CIFSR");
    wait_for_ok();

    sendATCommand("AT+CIPSTATUS");
    rc = wait_for_state(STATE_IP_STATUS);
    if (rc != 0) {
        printf("Failed to get IP state STATUS\n");
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf("state: IP STATUS\n");

    sendATCommand("AT+CIPSTART=0,\"TCP\",\"h.qartis.com\",\"53\"");   

    sendATCommand("AT+CIPSTATUS");
    rc = wait_for_state(STATE_CONNECTED);
    if (rc != 0) {
        if (state == STATE_IP_PROCESSING) {
            printf("connect: timeout\n");
            /* don't cancel the attempt. it might succeed later */
        } else {
            printf("connect: connection refused\n");
        }

        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf("state: CONNECTED\n");

    return 0; 
}
