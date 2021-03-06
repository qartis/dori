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

#include "free_ram.h"
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
#define FBUS_SMS_LOCATION_OFFSET 13
#define FBUS_SMS_CMD_OFFSET 9

#define FBUS_SUBSMS_INCOMING 0x04
#define FBUS_SUBSMS_SEND_STATUS 0x03

#define FBUS_SMS_MAX_LEN 128

#define PHONE_NUM_ASCII_LEN 10

volatile uint8_t sms_buf[FBUS_SMS_MAX_LEN];
volatile uint8_t sms_buflen;
char sms_dest[] = "7786860358";

volatile uint32_t modem_alive_time;
extern volatile uint8_t stfu;


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

char to_hex(uint8_t val)
{
    if (val < 10)
        return '0' + val;
    else
        return 'a' + (val - 10);
}

uint8_t startswith(const char *a, const char *b)
{
    return strncmp(a, b, strlen(b)) == 0;
}

uint8_t from_hex_byte(char a, char b)
{
    return (from_hex(a) << 4) | from_hex(b);
}

void powercycle(void)
{
    PORTD &= (1 << PORTD6);
    DDRD |= (1 << PORTD6);
    _delay_ms(2000);
    DDRD &= ~(1 << PORTD6);
}



ISR(USART_RX_vect)
{
    static uint8_t buf[140];
    static uint8_t buflen;

    enum fbus_frametype incoming_frametype;

    modem_alive_time = now;

    buf[buflen] = UDR0;

    /* if the header looks weird, resynchronize */
    if ((buflen == 0 && buf[buflen] != 0x1e) ||
        (buflen == 1 && buf[buflen] != 0x0c) ||
        (buflen == 2 && buf[buflen] != 0x00) ||
        (buflen == 5 && buf[buflen] > 128)) {
        //putchar('!');
        buflen = 0;
        return;
    }

    buflen++;

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
    uint8_t numframes = buf[FBUS_HEADER_LEN + len - 2];

    if (type == TYPE_ACK){
        printf_P(PSTR("got ack %x\n"), buf[FBUS_HEADER_LEN]);

        /*
        uint8_t i;
        for(i = 0; i < len; i++) {
            printf("02%f ", buf[i]);
        }
        printf("\n");
        uint8_t ack_type = buf[FBUS_HEADER_LEN];
        debug_putchar(to_hex((ack_type & 0xF0) >> 4));
        debug_putchar(to_hex((ack_type & 0x0F) >> 0));
        debug_putchar('\n');
        */
        fbus_ack_flag = 1;
        buflen = 0;
        return;
    }

    sendack(type, seq_no & 0x07);

    // ACKs don't contain seq_no and numframes
    if ((seq_no & 0xF0) != 0x40 || numframes != 1) {
        // multi frame
        puts_P(PSTR("multi"));
        /*
        debug_putchar('%');
        debug_putchar('%');
        */
        buflen = 0;
        return;
    }

    incoming_frametype = FRAME_UNKNOWN;

    uint8_t sms_cmd = buf[FBUS_SMS_CMD_OFFSET];
    if (type == TYPE_SMS) {
        printf_P(PSTR("sms_cmd: %0x, len: %d\n"), sms_cmd, len);
    }

    // SMS delivery reports and ID responses are used
    // to set flags, and don't require further processing
    // in the user_irq therefore set incoming_frametype to FRAME_NONE
    // so that the user_irq will never be triggered
    if (type == TYPE_SMS && sms_cmd == FBUS_SUBSMS_SEND_STATUS) {
        incoming_frametype = FRAME_NONE;
        uint8_t status = buf[14]; // could be 0, 1, (unknown = failure)
        printf_P(PSTR("@@@@@@sms status: %d\n"), status);
        if (status == 0) {
            fbus_sms_sent_flag = 1;
        }
    } else if (type == TYPE_ID){
        incoming_frametype = FRAME_NONE;
        puts_P(PSTR("fbus: id"));
        fbus_id_flag = 1;
    } else if (type == TYPE_SMS && sms_cmd == FBUS_SUBSMS_INCOMING){
        incoming_frametype = FRAME_SUBSMS_INCOMING;
    } else if (type == TYPE_NET_STATUS) {
        incoming_frametype = FRAME_NET_STATUS;
    }

    // Drop incoming frames that have lower priority than
    // what we're currently still processing
    // This means incoming SMS cannot interfere
    // an SMS that is still being processed by user_irq
    if (incoming_frametype > frametype) {
        frametype = incoming_frametype;
        fbustype = type;
        fbusseqno = seq_no;

        if (type == TYPE_SMS && sms_cmd == FBUS_SUBSMS_INCOMING) {
            uint8_t i;

            for (i = 0; i < buflen; i++)
                sms_buf[i] = buf[i];

            sms_buflen = buflen;
        }

        TRIGGER_USER1_IRQ();
    }

    buflen = 0;
}


void parse_sms(void)
{
    uint8_t i;
    uint8_t smstype = sms_buf[9];
    /* SUBSMS_INCOMING */
    if (smstype != 0x04) {
        //printf_P(PSTR("wtftype %d\n"), smstype);
        return;
    }

    uint8_t smstype2 = sms_buf[16];
    //uint8_t len = sms_buf[17];

    /* DELIVER */
    if (smstype2 != 0x00) {
        //puts_P(PSTR("wtf"));
        return;
    }

    uint8_t num_blocks = sms_buf[31];

    volatile uint8_t *block_ptr = sms_buf + 31 + 1;

    msg_buflen = 0;
    memset((char*)msg_buf, 0, MSG_BUF_SIZE);

    printf_P(PSTR("nm blks: %d\n"), num_blocks);
    for (i = 0; i < num_blocks; i++) {
        /* user data */
        switch (*block_ptr) {
        case 0x82:
            if (block_ptr[2] == 0x01) {
                unbcd_phonenum(phonenum_buf, block_ptr + 5, block_ptr[0]);
                printf("phone: '%s'\n", phonenum_buf);
            }
            break;
        case 0x80:
            msg_buflen = unpack7_msg(block_ptr + 4, block_ptr[3], msg_buf);
            printf("buflen: %d\n", msg_buflen);
            printf_P(PSTR("GOT MSG '%s'\n"), msg_buf);
            break;

        default:
            /* unknown block type `*block_ptr` */
            break;
        }

        block_ptr = block_ptr + block_ptr[1];
    }

    uint8_t id = sms_buf[FBUS_SMS_LOCATION_OFFSET];
    printf("del msg %d\n", id);
    fbus_delete_sms(id);
}

void process_can(uint8_t type, uint8_t id, uint16_t sensor, volatile uint8_t *data,
        uint8_t len)
{
    volatile uint8_t unbcd_buf[16];
    char output[32];
    uint8_t i;
    uint8_t j;

    switch(type) {
    case TYPE_value_request:
        if ((id == MY_ID || id == ID_any) && sensor == SENSOR_uptime) {

            if(stfu) {
                break;
            }

            uint8_t uptime_buf[4];
            uptime_buf[0] = uptime >> 24;
            uptime_buf[1] = uptime >> 16;
            uptime_buf[2] = uptime >> 8;
            uptime_buf[3] = uptime >> 0;

            mcp2515_send_sensor(TYPE_value_explicit, MY_ID, SENSOR_uptime, uptime_buf, sizeof(uptime_buf));

            output[0] = to_hex((TYPE_value_explicit & 0xf0) >> 4);
            output[1] = to_hex((TYPE_value_explicit & 0x0f) >> 0);

            output[2] = to_hex((MY_ID & 0xf0) >> 4);
            output[3] = to_hex((MY_ID & 0x0f) >> 0);

            output[4] = to_hex((SENSOR_uptime & 0xf000) >> 12);
            output[5] = to_hex((SENSOR_uptime & 0x0f00) >> 8);
            output[6] = to_hex((SENSOR_uptime & 0x00f0) >> 4);
            output[7] = to_hex((SENSOR_uptime & 0x000f) >> 0);

            output[8] = to_hex((4 & 0xf0) >> 4);
            output[9] = to_hex((4 & 0x0f) >> 0);

            for (i = 0, j = 0; i < sizeof(uptime_buf); i++, j += 2) {
                output[10 + j] = to_hex((uptime_buf[i] & 0xF0) >> 4);
                output[11 + j] = to_hex((uptime_buf[i] & 0x0F) >> 0);
            }

            output[10 + j] = '\0';

            printf("out: '%s'\n", output);

            fbus_sendsms(sms_dest, output);
        }
        break;
    case TYPE_action_modemb_powercycle:
        powercycle();
        break;
    case TYPE_action_modemb_wipesms:
        for (i = 0; i < 32; i++) {
            fbus_delete_sms(i);
        }
        break;
    case TYPE_set_sms_dest:
        unbcd_buf[0] = 0;

        for (i = 0; i < len; i++) {
            unbcd_buf[i + 1] = (data[i] & 0xf0) >> 4 | (data[i] & 0x0f) << 4;
        }

        unbcd_phonenum(sms_dest, unbcd_buf, PHONE_NUM_ASCII_LEN);
        printf("dest: %s\n", sms_dest);
        fbus_sendsms(sms_dest, "00");
        break;
    case TYPE_sos_nopsled:
        mcp2515_send_sensor(TYPE_nop, 0, 0, (const uint8_t *)"\0\0\0\0\0\0\0\0", 8);
        break;
    }

}



uint8_t user1_irq(void)
{
    uint8_t rc;
    uint8_t type, id, len;
    uint16_t sensor;
    uint8_t i;
    uint8_t j;
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
        printf("len:%d\n", msg_buflen);

        while (msg_buflen >= 10) {
            // convert SMS ascii hex to CAN
            // [type, id, sensor (16 bits), len, <data> ]
            // must be at least 5 bytes, 10 characters total
            type = from_hex_byte(msg[0], msg[1]);
            id = from_hex_byte(msg[2], msg[3]);

            sensor = from_hex_byte(msg[4], msg[5]) << 8;
            sensor |= from_hex_byte(msg[6], msg[7]);

            len = from_hex_byte(msg[8], msg[9]);

            printf_P(PSTR("pkt: %02x %02x %04x %02x\n"), type, id, sensor, len);

            if (len > 8) { // invalid length
                printf("inval len: %d\n", len);
                break;
            }

            for (i = 0, j = 0; j < len; j++, i+=2) {
                //printf("%c",  msg[10 + i]);
                //printf("%c ", msg[11 + i]);
                data[j] = from_hex_byte(msg[10 + i], msg[11 + i]);
            }

            printf("snd can\n");

            mcp2515_send_wait(type, id, sensor, data, len);

            process_can(type, id, sensor, data, len);

            if (msg_buflen < 10 + (len * 2)) {
                puts_P(PSTR("inval sms"));
                rc = 0;
                break;
            }

            // size of the last read CAN msg from the SMS
            msg_buflen -= 10 + (len * 2);
            msg += 10 + (len * 2);
        }

        // delete the msg after it's sent,
        // but only after someone on CAN has acked it
        break;

    case FRAME_ID:
        puts_P(PSTR("wtf693?"));
        break;

    case FRAME_NET_STATUS:
        putchar('$');
        break;

    case FRAME_SUBSMS_SEND_STATUS:
        puts_P(PSTR("wtf12"));
        break;

    default:
        printf_P(PSTR("user_irq: %d\n"), frametype);
        break;
    }

    frametype = FRAME_NONE;

    return rc;
}

uint8_t debug_irq(void)
{
    char buf[64];
    uint8_t rc;
    fgets(buf, sizeof(buf), stdin);
    debug_flush();
    buf[strlen(buf)-1] = '\0';

    if (strcmp(buf, "on") == 0) {
        powercycle();
    } else if (strcmp(buf, "off") == 0) {
        powercycle();
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
        strtok(buf, " ");
        const char *num_ptr = strtok(NULL, " ");
        const char *msg_ptr = strtok(NULL, " ");
        if (!num_ptr || !msg_ptr)
            return 0;

        if (startswith(num_ptr, "dori"))
            num_ptr = "6046529453";
            //num_ptr = "6043524947";
        else if (startswith(num_ptr, "v"))
            num_ptr = "7788969633";

        if(startswith(msg_ptr, "echo"))
            msg_ptr = "010200010401020304";

        rc = fbus_heartbeat(); // send ID before any SMS
        if (rc)
            goto done;

        rc = fbus_sendsms(num_ptr, msg_ptr);
        if (rc)
            goto done;
    } else if (startswith(buf, "del")) {
        uint16_t loc = 0;
        sscanf(&buf[strlen("del ")], "%d", &loc);
        fbus_delete_sms(loc);
    } else if (strcmp(buf, "sub") == 0) {
        rc = fbus_subscribe();
        printf("rc: %d\n", rc);
    } else if (strcmp(buf, "id") == 0) {
        fbus_heartbeat();
    } else if (strcmp(buf, "UU") == 0) {
        fbus_init();
        fbus_init();
        fbus_init();
    } else if (strcmp(buf, "mem") == 0) {
        printf("spc: %x\n", free_ram());
    } else {
        printf_P(PSTR("got '%s'\n"), buf);
    }

done:
    debug_flush();
    return 0;
}


uint8_t periodic_irq(void)
{
    uint8_t rc;

    if (now - modem_alive_time >= 10) {
        rc = fbus_subscribe();

        if (rc != 0) {
            powercycle();

            _delay_ms(4000);
            fbus_subscribe();
        } else {
            printf("fbus is alive\n");
        }
    } else {
        printf("not yet...\n");
    }

    return 0;
}

uint8_t can_irq(void)
{
    uint8_t rc;
    uint8_t i;
    uint8_t j;
    char output[140];

    printf_P(PSTR("can_irq\n"));

    while (mcp2515_get_packet(&packet) == 0) {
        process_can(packet.type, packet.id, packet.sensor, packet.data, packet.len);

        if (packet.type == TYPE_value_request) {
            /* ignore */
            continue;
        } else if (stfu) {
            printf("stfuing\n");
            continue;
        }

        output[0] = to_hex((packet.type & 0xf0) >> 4);
        output[1] = to_hex((packet.type & 0x0f) >> 0);

        output[2] = to_hex((packet.id & 0xf0) >> 4);
        output[3] = to_hex((packet.id & 0x0f) >> 0);

        output[4] = to_hex((packet.sensor & 0xf000) >> 12);
        output[5] = to_hex((packet.sensor & 0x0f00) >> 8);
        output[6] = to_hex((packet.sensor & 0x00f0) >> 4);
        output[7] = to_hex((packet.sensor & 0x000f) >> 0);

        output[8] = to_hex((packet.len & 0xf0) >> 4);
        output[9] = to_hex((packet.len & 0x0f) >> 0);

        printf_P(PSTR("%02x%02x%04x%02x"), packet.type, packet.id, packet.sensor,
                packet.len);
        putchar('\n');

        for (i = 0, j = 0; i < packet.len; i++, j += 2) {
            output[10 + j] = to_hex((packet.data[i] & 0xF0) >> 4);
            output[11 + j] = to_hex((packet.data[i] & 0x0F) >> 0);
        }

        output[10 + j] = '\0';

        printf_P(PSTR("snd'%s'\n"), output);
        rc = fbus_sendsms(sms_dest, output);

        if (rc) {
            puts_P(PSTR("problem sending SMS"));
        } else {
            puts_P(PSTR("sent ok"));
        }
    }

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
    uint8_t rc;

    node_init();

    stfu = 1;

    sei();

    rc = fbus_subscribe();

    if (rc != 0) {
        powercycle();

        _delay_ms(4000);
        fbus_subscribe();
    } else {
        printf("fbus is alive\n");
    }

    node_main();
}
