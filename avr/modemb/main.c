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

#include "fbus.h"
#include "can.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "node.h"
#include "debug.h"
#include "irq.h"

#define FBUS_HEADER_LEN 6
#define FBUS_CHECKSUM_LEN 2
#define FBUS_PHONE_NUM_OFFSET 29
#define FBUS_MSG_OFFSET 48
#define FBUS_MSG_SIZE_OFFSET 28
#define FBUS_SMS_MEM_TYPE_OFFSET 10
#define FBUS_SMS_LOCATION_OFFSET 11
#define FBUS_SMS_CMD_OFFSET 9

#define FBUS_SMS_CMD_DEL 0x0A
//#define FBUS_SMS_CMD_RECV 0x10
#define FBUS_SMS_CMD_RECV 0x04

#define FBUS_SMS_CMD_SEND 0x01
#define FBUS_SMS_CMD_SENT_OK 0x02
#define FBUS_SMS_CMD_ERROR 0x03

#define FBUS_PHONE_NUM_LEN 12
#define FBUS_SMS_MAX_LEN 128

volatile enum fbus_frametype frametype;

volatile uint8_t fbusseqno;
volatile uint8_t fbustype;

volatile uint8_t sms_buf[FBUS_SMS_MAX_LEN];
volatile uint8_t sms_buflen;

volatile uint8_t phone_num_buf[FBUS_PHONE_NUM_LEN];
//volatile uint8_t phone_num_buflen;

uint8_t uart_irq(void)
{
    return 0;
}

ISR(USART_RX_vect)
{
    static uint8_t buf[128];
    static uint8_t buflen;

    enum fbus_frametype incoming_frametype;

    buf[buflen] = UDR0;

    /* if the header looks weird, resynchronize */
    if ((buflen == 0 && buf[buflen] != 0x1e) ||
        (buflen == 1 && buf[buflen] != 0x0c) ||
        (buflen == 2 && buf[buflen] != 0x00) ||
        (buflen == 5 && buf[buflen] > 128)) {
        putchar('!');
        buflen = 0;
        return;
    }

    buflen++;
//    printf("2%d\n", buflen);

    /* if we don't know the length yet */
    if (buflen < FBUS_HEADER_LEN)
        return;

    uint8_t len = buf[5];
    uint8_t type = buf[3];
    uint8_t padding = len % 2;

    /* while we're reading `len` bytes of data */
    if (buflen < FBUS_HEADER_LEN + len + padding + FBUS_CHECKSUM_LEN) return;

    /* seq no is the last byte of the data */
    uint8_t seq_no = buf[FBUS_HEADER_LEN + len - 1];

    if (type == TYPE_ACK){
        buflen = 0;
        return;
    }

    uint8_t sms_cmd = buf[FBUS_SMS_CMD_OFFSET]; 
    if (type == TYPE_SMS && sms_cmd == FBUS_SMS_CMD_RECV){
        incoming_frametype = FRAME_SMS_RECV;
        /*
    } else if (type == TYPE_SMS && sms_cmd == FBUS_SMS_CMD_SENT_OK){
        incoming_frametype = FRAME_SMS_SENT;
    } else if (type == TYPE_SMS && sms_cmd == FBUS_SMS_CMD_ERROR){
        incoming_frametype = FRAME_SMS_ERROR;
        */
    } else if (type == TYPE_ID){
        incoming_frametype = FRAME_ID;
    } else if (type == TYPE_NET_STATUS){
        incoming_frametype = FRAME_NET_STATUS;
    } else {
        incoming_frametype = FRAME_UNKNOWN;
    }

    if (incoming_frametype > frametype) {
        frametype = incoming_frametype;
        fbustype = type;
        fbusseqno = seq_no;

        if (type == TYPE_SMS && sms_cmd == FBUS_SMS_CMD_RECV) {
            uint8_t i;

            /*
            for (i = 0; i < FBUS_PHONE_NUM_LEN; i++)
                phone_num_buf[i] = buf[FBUS_PHONE_NUM_OFFSET + i];
            */

            for (i = 0; i < buflen; i++)
                sms_buf[i] = buf[i];

            sms_buflen = buflen;
        }

        TRIGGER_USER_IRQ();
    }
    //printf("6");

    buflen = 0;
}

void parse_sms(void)
{
    uint8_t i;
    uint8_t smstype = sms_buf[9];
    /* SUBSMS_INCOMING */
    if (smstype != 0x04) {
        printf("wrong sms type %d\n", smstype);
        return;
    }

    uint8_t smstype2 = sms_buf[16];
    //uint8_t len = sms_buf[17];

    /* DELIVER */
    if (smstype2 != 0x00) {
        printf("wtf\n");
        return;
    }

    uint8_t num_blocks = sms_buf[31];

    volatile uint8_t *block_ptr = sms_buf + 31 + 1;

    for (i = 0; i < num_blocks; i++) {
        /* user data */
        //printf("block type %x\n", *block_ptr);
        switch (*block_ptr) {
        case 0x82:
            if (block_ptr[2] == 0x01) {
                unbcd_phonenum(block_ptr + 4);
                printf("phone: '%s'\n", phonenum_buf);
            }
            break;

        case 0x80:
            unpack7_msg(block_ptr + 4, block_ptr[2]);
            msg_buf[6] = '\0';
            printf("GOT MSG '%s'\n", msg_buf);
            break;

        default:
            /* unknown block type `*block_ptr` */
            break;
        }

        block_ptr = block_ptr + block_ptr[1];
    }

    //fbus_delete_sms(buf[FBUS_SMS_MEM_TYPE_OFFSET], buf[FBUS_SMS_LOCATION_OFFSET]);
}

uint8_t user_irq(void)
{
    sendack(fbustype, fbusseqno & 0x07);

    switch (frametype) {
    case FRAME_SMS_RECV:
        parse_sms();
        break;

    case FRAME_ID:
        printf("fbus: id\n");
        break;

    case 3:
        putchar('$');
        printf("%ld\n", now);
        break;

    default:
        printf("user_irq: %d\n", frametype);
    }

    frametype = FRAME_NONE;

    return 0;
}


uint8_t debug_irq(void)
{
    char buf[64];
    fgets(buf, sizeof(buf), stdin);
    debug_flush();
    buf[strlen(buf)-1] = '\0';

    if (strcmp(buf, "on") == 0) {
        PORTD &= (1 << PORTD6);
        DDRD |= (1 << PORTD6);
        _delay_ms(1000);
        DDRD &= ~(1 << PORTD6);
    } else if (strcmp(buf, "pwr") == 0) {
        PORTD &= (1 << PORTD6);
        DDRD |= (1 << PORTD6);
        _delay_ms(1000);
        DDRD &= ~(1 << PORTD6);

        _delay_ms(5000);
        _delay_ms(4000);

        PORTD &= (1 << PORTD6);
        DDRD |= (1 << PORTD6);
        _delay_ms(1000);
        DDRD &= ~(1 << PORTD6);
    } else if (strcmp(buf, "sms") == 0) {
    } else if (strcmp(buf, "sub") == 0) {
        fbus_subscribe();
    } else if (strcmp(buf, "id") == 0) {
        fbus_heartbeat();
    }else {
        printf("got '%s'\n", buf);
    }

    PROMPT;

    return 0;
}

uint8_t periodic_irq(void)
{
    return 0;
}

uint8_t can_irq(void)
{
    packet.unread = 0;

    return 0;
}

void main(void)
{
    NODE_INIT();

    sei();

    NODE_MAIN();
}
