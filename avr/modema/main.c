#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "node.h"
#include "sim900.h"
#include "debug.h"
#include "irq.h"

#define streq(a,b) (strcmp(a,b) == 0)
#define strstart(a,b) (strncmp(a,b,strlen(b)) == 0)

uint8_t uart_irq(void)
{
    return 0;
}

uint8_t user_irq(void)
{
    uint8_t rc;

    printf("connecting\n");
    rc = TCPConnect();
    if (rc)
        printf("connect: %d\n", rc);
    return 0;
}

uint8_t debug_irq(void)
{
    char buf[64];
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf)-1] = '\0';
    printf("got '%s'\n", buf);

    if (streq(buf, "c")) {
        irq_signal |= IRQ_USER;
    } else {
        print(buf);
        uart_putchar('\r');
    }

    _delay_ms(500);
    PROMPT;

    return 0;
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
        putchar('X');
        tcp_rx_buf[tcp_rx_buf_len] = c;
        tcp_rx_buf[tcp_rx_buf_len + 1] = '\0';
        tcp_rx_buf_len++;
        tcp_toread--;
        if (tcp_toread == 0) {
            printf("Received TCP chunk: '%s'\n", tcp_rx_buf);
            tcp_rx_buf_len = 0;
        }

        return;
    }


    if (state == STATE_EXPECTING_PROMPT && at_buf[0] == '>') {
        state = STATE_GOT_PROMPT;
        at_buf_len = 0;
        at_buf[0] = '\0';
    } else if (c == '\r') {
        //puts(at_buf);
        //putchar('\r');
        //putchar('\n');
        if (streq(at_buf, "OK")) {
            flag_ok = 1;
            /* We don't care anymore */
        } else if (streq(at_buf, "ERROR")) {
            flag_error = 1;
            /* We don't care anymore */
        } else if (streq(at_buf, "STATE: IP INITIAL")) {
            state = STATE_IP_INITIAL;
        } else if (streq(at_buf, "STATE: IP START")) {
            state = STATE_IP_START;
        } else if (streq(at_buf, "STATE: IP GPRSACT")) {
            state = STATE_IP_GPRSACT;
        } else if (streq(at_buf, "STATE: IP STATUS")) {
            state = STATE_IP_STATUS;
        } else if (streq(at_buf, "STATE: IP PROCESSING")) {
            state = STATE_IP_PROCESSING;
        } else if (streq(at_buf, "0, CONNECT OK")) {
            state = STATE_CONNECTED;
        } else if (streq(at_buf, "0, CONNECT FAIL")) {
            state = STATE_ERROR;
        } else if (streq(at_buf, "0, SEND OK")) {
            state = STATE_CONNECTED;
        } else if (streq(at_buf, "0, CLOSE OK")) {
            state = STATE_CLOSED;
        } else if (streq(at_buf, "0, CLOSED")) {
            printf("connection closed by peer\n");
            state = STATE_CLOSED;
        } else if (streq(at_buf, "NORMAL POWER DOWN")) {
            state = STATE_POWEROFF;
        } else if (strstart(at_buf, "+CME ERROR:")) {
            state = STATE_ERROR;
        } else if (streq(at_buf, "SHUT OK")) {
            state = STATE_CLOSED;
        } else if (strstart(at_buf,"+RECEIVE,0,")) {
            uint8_t len = atoi(at_buf + strlen("+RECEIVE,0,"));
            printf("TCP: Going to read %d bytes\n", len);
            tcp_toread = len;

            /* modem just sent +RECEIVE,0,%d\r\n. we must chomp the \n */
        } else if (streq(at_buf, "RING")) {
            putchar('@');
            slow_write("ATH\r", strlen("ATH\r"));
            _delay_ms(1500);
            /* trigger remote connect() */
            irq_signal |= IRQ_USER;
        }

        at_buf_len = 0;
        at_buf[0] = '\0';
    } else if (c != '\n' && c != '\0') {
        if (at_buf_len < sizeof(at_buf) - 5) {
            at_buf[at_buf_len] = c;
            at_buf[at_buf_len + 1] = '\0';
            at_buf_len++;
        } else {
            putchar('X');
            putchar('X');
        }
    }
}

uint8_t periodic_irq(void)
{
    /* send a heartbeat to let modemb know we're alive */
#if 0
    rc = mcp2515_send(TYPE_value_periodic, ID_modemb, 3, buf);
    if (rc != 0) {
        return rc;
    }
#endif

    return 0;
}

uint8_t can_irq(void)
{
    switch (packet.type) {
    default:
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
