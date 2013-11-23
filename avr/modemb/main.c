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

#define FBUS_SUBSMS_INCOMING 0x04
#define FBUS_SUBSMS_SEND_STATUS 0x03

#define FBUS_PHONE_NUM_LEN 12
#define FBUS_SMS_MAX_LEN 128

volatile enum fbus_frametype frametype;

volatile uint8_t fbusseqno;
volatile uint8_t fbustype;

volatile uint8_t sms_buf[FBUS_SMS_MAX_LEN];
volatile uint8_t sms_buflen;

volatile uint8_t phone_num_buf[FBUS_PHONE_NUM_LEN];
//volatile uint8_t phone_num_buflen;


uint8_t from_hex(char a)
{
    if (isupper(a)) {
        return a - 'A' + 10;
    } else if (islower(a)) {
        return a - 'a' + 10;
    } else {
        return a - '0';
    }
}

uint8_t startswith(const char *a, const char *b)
{
    return strncmp(a, b, strlen(b)) == 0;
}

uint8_t from_hex_8(char a, char b)
{
    return (from_hex(a) << 4) | from_hex(b);
}

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
    if (type == TYPE_SMS && sms_cmd == FBUS_SUBSMS_INCOMING){
        incoming_frametype = FRAME_SUBSMS_INCOMING;
    } else if (type == TYPE_SMS && sms_cmd == FBUS_SUBSMS_SEND_STATUS) {
        incoming_frametype = FRAME_SUBSMS_SEND_STATUS;
    } else if (type == TYPE_ID){
        incoming_frametype = FRAME_ID;
    } else if (type == TYPE_NET_STATUS){
        incoming_frametype = FRAME_NET_STATUS;
    } else {
        incoming_frametype = FRAME_UNKNOWN;
    }
    
    //printf("ift: %d, ft:%d\n", incoming_frametype, frametype);

    if (incoming_frametype > frametype) {
        frametype = incoming_frametype;
        fbustype = type;
        fbusseqno = seq_no;

        if (type == TYPE_SMS && sms_cmd == FBUS_SUBSMS_INCOMING) {
            uint8_t i;

            /*
            for (i = 0; i < FBUS_PHONE_NUM_LEN; i++)
                phone_num_buf[i] = buf[FBUS_PHONE_NUM_OFFSET + i];
            */

            for (i = 0; i < buflen; i++)
                sms_buf[i] = buf[i];

            sms_buflen = buflen;
        }
        else if(type == TYPE_SMS && sms_cmd == FBUS_SUBSMS_SEND_STATUS) {
            uint8_t status = buf[14]; // could be 0, 1, (unknown = failure)
            if(status == 0) {
                // success
                //uint8_t ref_num = buf[16]; // sms reference
            }
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

    msg_buflen = 0;
    memset((char*)msg_buf, 0, MSG_BUF_SIZE);

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
            {
                uint8_t j;
                for (j = 0; j < block_ptr[3]; j++) {
                    printf("%d: %x\n", j, block_ptr[4 + j]);
                }

            }
            msg_buflen = unpack7_msg(block_ptr + 4, block_ptr[3], &msg_buf[msg_buflen]);
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
    uint8_t rc;
    sendack(fbustype, fbusseqno & 0x07);

    uint8_t type, id, len;
    uint16_t sensor;
    uint8_t i;

    uint8_t data[16] = { 0 };

    type = 0;
    id = 0;
    sensor = 0;
    len = 0;

    rc = 0;

    // !!!! Do not return, break instead to fall through
    // !!!! to the frametype handling below
    switch (frametype) {
    case FRAME_SUBSMS_INCOMING:
        parse_sms();

        volatile char *msg = msg_buf;

        while (msg_buflen >= 10) {
            // convert SMS ascii hex to CAN
            // [type, id, sensor (16 bits), len, <data> ]
            // must be at least 5 bytes, 10 characters total
            type = from_hex_8(msg[0], msg[1]);
            id = from_hex_8(msg[2], msg[3]);

            sensor = from_hex_8(msg[4], msg[5]) << 8;
            sensor |= from_hex_8(msg[6], msg[7]);

            len = from_hex_8(msg[8], msg[9]);

            if(len > 8) { // invalid length
                break;
            }

            printf("pkt: %02x %02x %04x %02x: ", type, id, sensor, len);

            uint8_t j = 0;
            for(i = 0; j < len; j++, i+=2) {
                printf("%c",  msg[10 + i]);
                printf("%c ", msg[11 + i]);
                data[j] = from_hex_8(msg[10 + i],
                                     msg[11 + i]);
            }
            printf("\n");

            rc = mcp2515_send_wait(type, id, data, len, sensor);

            printf("msg_buflen: %d\n", msg_buflen);
            if(msg_buflen < 10 + (len * 2)) {
                printf("inval sms\n");
                rc = 0;
                break;
            }

            // size of the last read CAN msg from the SMS
            msg_buflen -= 10 + (len * 2);
            msg += 10 + (len * 2);
        }

        // uint8_t mcp2515_send_wait(uint8_t type, uint8_t id, const void *data, uint8_t len, uint16_t sensor)
        // send it over CAN
        // delete the msg after it's sent,
        // but only after someone on CAN has acked it
        break;

    case FRAME_ID:
        printf("fbus: id\n");
        break;

    case FRAME_NET_STATUS:
        putchar('$');
        printf("%ld\n", now);
        break;

    default:
        printf("user_irq: %d\n", frametype);
    }

    frametype = FRAME_NONE;

    return rc;
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
    } else if (startswith(buf, "sms")) {
        fbus_heartbeat(); // send ID before any SMS
        strtok(buf, " ");
        const char *num_ptr = strtok(NULL, " ");
        const char *msg_ptr = strtok(NULL, " ");
        if(num_ptr && msg_ptr) {
            if(startswith(num_ptr, "dori")) {
                num_ptr = "6043524947";
            }
            else if(startswith(num_ptr, "v")) {
                num_ptr = "7788969633";
            }
            if(startswith(msg_ptr, "echo")) {
                msg_ptr = "010200010401020304";
            }
            //printf("Sending '%s' to '%s'\n", msg_ptr, num_ptr);
            fbus_sendsms(num_ptr, msg_ptr);
        }
    } else if (strcmp(buf, "sub") == 0) {
        fbus_subscribe();
    } else if (strcmp(buf, "id") == 0) {
        fbus_heartbeat();
    } else if (strcmp(buf, "UU") == 0) {
        fbus_init();
        fbus_init();
        fbus_init();
    }else {
        printf("got '%s'\n", buf);
    }

    //printf("cur frametype: %d\n", frametype);

    PROMPT;

    return 0;
}

uint8_t periodic_irq(void)
{
    return 0;
}

uint8_t can_irq(void)
{
    printf("can_irq\n");
    char output[140] = { 0 };
    sprintf(output, "%02x%02x%04x%02x", packet.type, packet.id, packet.sensor, packet.len);
    printf("%02x%02x%04x%02x\n", packet.type, packet.id, packet.sensor, packet.len);

    uint8_t i;
    uint8_t j;
    for(i = 0, j = 0; i < packet.len; i++, j+=2) {
        sprintf(&output[10 + j], "%0x", (packet.data[i] & 0xF0) >> 4);
        sprintf(&output[11 + j], "%0x", packet.data[i] & 0x0F);
    }

    fbus_heartbeat(); // send ID before any SMS

    printf("sending '%s'\n", output);
    //fbus_sendsms("7788969633", output);
    fbus_sendsms("7786860358", output);


    packet.unread = 0;

    return 0;
}

void main(void)
{
    NODE_INIT();

    sei();

    NODE_MAIN();
}
