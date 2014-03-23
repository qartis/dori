#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "timer.h"
#include "fbus.h"
#include "uart.h"

volatile enum fbus_frametype frametype;
volatile uint8_t fbus_ack_flag;
volatile uint8_t fbus_sms_sent_flag;
volatile uint8_t fbus_id_flag;

volatile uint8_t fbusseqno;
volatile uint8_t fbustype;

void _delay_ms_flag(uint16_t msec, volatile uint8_t * flag)
{
    while (*flag == 0 && --msec) {
        _delay_ms(1);
    }
}

void print_len(const uint8_t * buf, uint8_t len)
{
    while (len--) {
        uart_putchar(*buf++);
    }
}

static uint8_t table[] = {
    /* ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet */
    '@', 0xa3, '$', 0xa5, 0xe8, 0xe9, 0xf9, 0xec,
    0xf2, 0xc7, '\n', 0xd8, 0xf8, '\r', 0xc5, 0xe5,
    '?', '_', '?', '?', '?', '?', '?', '?',
    '?', '?', '?', '?', 0xc6, 0xe6, 0xdf, 0xc9,
    ' ', '!', '\"', '#', 0xa4, '%', '&', '\'',
    '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', ':', ';', '<', '=', '>', '?',
    0xa1, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
    'X', 'Y', 'Z', 0xc4, 0xd6, 0xd1, 0xdc, 0xa7,
    0xbf, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
    'x', 'y', 'z', 0xe4, 0xf6, 0xf1, 0xfc, 0xe0
};

uint8_t bcd(uint8_t * dest, const char *s)
{
    uint8_t x, y, n, hi, lo;

    x = 0;
    y = 0;
    if (*s == '+') {
        s++;
    }
    while (s[x]) {
        lo = s[x++] - '0';
        if (s[x]) {
            hi = s[x++] - '0';
        } else {
            hi = 0x0f;
        }

        n = (hi << 4) + lo;
        dest[y++] = n;
    }
    return y;
}

volatile char phonenum_buf[16];
void unbcd_phonenum(volatile char *dest, volatile uint8_t *data, uint8_t len)
{
    uint8_t n, x, at;

    if (data[0] == 0x6f || data[0] == 0x91) {
        *dest++ = '+';
    }

    at = 1;
    for (n = 0; n < len; ++n) {
        x = data[at] & 0x0f;

        if (x < 10) {
            *dest++ = '0' + x;
        }

        n++;

        if (n >= len) {
            break;
        }

        x = (data[at] >> 4) & 0x0f;

        if (x < 10) {
            *dest++ = '0' + x;
        }

        at++;
    }

    *dest = '\0';
}

uint8_t escaped(uint8_t c)
{
    switch (c) {
    case 0x0a:
        return '\n';
    case 0x14:
        return '^';
    case 0x28:
        return '{';
    case 0x29:
        return '}';
    case 0x2f:
        return '\\';
    case 0x3c:
        return '[';
    case 0x3d:
        return '~';
    case 0x3e:
        return ']';
    case 0x40:
        return '|';
    default:
        return '?';
    }
}

volatile char msg_buf[MSG_BUF_SIZE];
volatile uint8_t msg_buflen;
uint8_t unpack7_msg(volatile uint8_t * data, uint8_t len, volatile char *output)
{
    uint16_t *p, w;
    uint8_t c;
    uint8_t n;
    uint8_t shift = 0;
    uint8_t at = 0;
    uint8_t escape = 0;
    volatile char *endptr = output;

    for (n = 0; n < len; ++n) {
        p = (uint16_t *) (data + at);
        w = *p;
        w >>= shift;
        c = w & 0x7f;

        shift += 7;
        if (shift & 8) {
            shift &= 0x07;
            at++;
        }

        if (escape) {
            *endptr++ = escaped(c);
            escape = 0;
        } else if (c == 0x1b) {
            escape = 1;
        } else {
            *endptr++ = table[c];
        }
    }

    *endptr = '\0';

    return len;
}

uint8_t gettrans(uint8_t c)
{
    uint8_t n;

    if (c == '?') {
        return '?';
    }

    for (n = 0; n < 128; ++n) {
        if (table[n] == c) {
            return n;
        }
    }

    return '?';
}

uint8_t pack7(uint8_t * dest, const char *s)
{
    uint8_t len;
    uint16_t *p, w;
    uint8_t at;
    uint8_t shift;
    uint8_t n, x;

    len = strlen(s);
    x = ((uint16_t) (len + 1) * 7) / 8;

    for (n = 0; n < x; ++n) {
        dest[n] = 0;
    }

    shift = 0;
    at = 0;
    w = 0;
    for (n = 0; n < len; n++) {
        p = (uint16_t *) (dest + at);
        w = gettrans(s[n]) & 0x7f;
        w <<= shift;

        *p |= w;

        shift += 7;
        if (shift >= 8) {
            shift &= 7;
            at++;
        }
    }

    /* return packed length */
    return x;
}

void sendframe(uint8_t type, uint8_t * data, uint8_t size)
{
    uint8_t i;
    uint8_t check[2];

    uart_putchar(0x1e);
    uart_putchar(0x00);
    uart_putchar(0x0c);
    uart_putchar(type);
    uart_putchar(0x00);
    uart_putchar(size);

    check[0] = 0x1e ^ 0x0c ^ 0x00;
    check[1] = 0x00 ^ type ^ size;

    for (i = 0; i < size; i++) {
        uart_putchar(data[i]);
        check[i & 1] ^= data[i];
    }

    /* if odd numbered, add filler byte */
    if (size % 2) {
        uart_putchar(0x00);
    }

    uart_putchar(check[0]);
    uart_putchar(check[1]);
}

uint8_t sendframe_wait(uint8_t type, uint8_t * data, uint8_t size)
{
    uint8_t retry;

    fbus_ack_flag = 0;
    retry = 3;
    while (fbus_ack_flag == 0 && --retry) {
        sendframe(type, data, size);
        _delay_ms_flag(500, &fbus_ack_flag);
    }

    if (fbus_ack_flag == 0) {
        puts_P(PSTR("NOACK!"));
    }

    return !(fbus_ack_flag == 1);
}

void sendack(uint8_t type, uint8_t seqnum)
{
    uint8_t buf[2];

    buf[0] = type;
    buf[1] = seqnum;

    sendframe(TYPE_ACK, buf, sizeof(buf) / sizeof(buf[0]));
}

void fbus_init(void)
{
    uint8_t c;

    for (c = 0; c < 128; c++) {
        uart_putchar('U');
    }

    uart_tx_flush();
    _delay_ms(5);
}

uint8_t uart_sendsms(const char *num, const char *ascii)
{
    uint8_t rc;
    uint8_t retry;
    uint8_t packet_len_idx;
    uint8_t buf[128];
    uint8_t len = 0;

    buf[len++] = 0x00;
    buf[len++] = 0x01;
    buf[len++] = 0x00;          //SMS frame header

    buf[len++] = 0x02;
    buf[len++] = 0x00;
    buf[len++] = 0x00;          //send SMS message

    buf[len++] = 0x00;
    buf[len++] = 0x55;
    buf[len++] = 0x55;

    memset(buf + len, 0, 100);

    buf[len++] = 0x01;
    buf[len++] = 0x02;

    packet_len_idx = len;
    buf[len++] = 0x00;          // NUL, will be set later

    buf[len++] = 0x11;          // hex field for sms properties
    buf[len++] = 0x00;          // reference
    buf[len++] = 0x00;          // PID
    buf[len++] = 0x00;          // DCS
    buf[len++] = 0x00;
    buf[len++] = 0x04;          // total blocks

    buf[len++] = 0x82;
    buf[len++] = 0x0c;
    buf[len++] = 0x01;          // remote number
    buf[len++] = 0x07;          // actual data length in this block

    buf[len++] = strlen(num);   // length of phone number (unencoded)
    buf[len++] = 0x81;          // type - unknown

    len += bcd(buf + len, num);

    buf[len++] = 0x00;          // start of SMSC block
    buf[len++] = 0x82;
    buf[len++] = 0x0c;
    buf[len++] = 0x02;          // field type: SMSC number
    buf[len++] = 0x08;

    buf[len++] = 0x07;          // start of SMSC number
    buf[len++] = 0x91;          // type - international
    buf[len++] = 0x71;          // 1-705-796-9300
    buf[len++] = 0x50;
    buf[len++] = 0x97;
    buf[len++] = 0x96;
    buf[len++] = 0x03;
    buf[len++] = 0xf0;

    buf[len++] = 0x80;          // start of user data block

    uint8_t enc_len = pack7(buf + len + 3, ascii);
    uint8_t len_field_idx = len;

    buf[len++] = enc_len + 4;   // "length + 4"
    buf[len++] = enc_len;       // encoded length of sms msg
    buf[len++] = strlen(ascii); // un-encoded length of sms msg

    len += enc_len;

    if (buf[len_field_idx] % 8 != 0) {
        uint8_t pad_len = 8 - (buf[len_field_idx] % 8);
        uint8_t i;
        for (i = 0; i < pad_len; i++) {
            buf[len++] = 0x55;
        }

        buf[len_field_idx] += pad_len;
    }

    buf[len++] = 0x08;
    buf[len++] = 0x04;
    buf[len++] = 0x01;
    buf[len++] = 0xa9;          // validity

    buf[packet_len_idx] = len - 9 - 1;

    buf[len++] = 0x01;          // terminator
    buf[len++] = 0x41;          // seq no

    // Now we keep trying to send the SMS
    // until we get a successful delivery report
    // If we get a failure delivery report
    // then fbus_sms_sent_flag will still be 0
    fbus_sms_sent_flag = 0;
    retry = 2;
    while (fbus_sms_sent_flag == 0 && --retry) {
        rc = sendframe_wait(TYPE_SMS, buf, len);
        if (rc)
            return 99;

        _delay_ms_flag(5000, &fbus_sms_sent_flag);
        _delay_ms_flag(5000, &fbus_sms_sent_flag);
        _delay_ms_flag(5000, &fbus_sms_sent_flag);
    }

    if (fbus_sms_sent_flag == 0) {
        puts_P(PSTR("NOSENT!"));
    }

    return fbus_sms_sent_flag == 0;
}

uint8_t fbus_sendsms(const char *num, const char *msg)
{
    fbus_init();
    return uart_sendsms(num, msg);
}

uint8_t fbus_delete_sms(uint16_t loc)
{
    uint8_t rc;
    unsigned char del_sms[] = {
        0x00, 0x01, 0x00,       // FBUS_FRAME_HEADER,
        0x04,
        0x01,                   /* 0x01 for SM (inbox is on SIM), 0x02 for ME */
        0x02,                   /* folder_id */
        (loc >> 8) & 0xFF,      /* Location Hi */
        loc & 0xFF,             //0x00, /* Location Lo */
        0x0F,
        0x55,
        0x01,
        0x44,
    };

    rc = fbus_heartbeat();
    if (rc)
        return 63;

    return sendframe_wait(TYPE_SMS_MGMT, del_sms, sizeof(del_sms));
}

uint8_t fbus_subscribe(void)
{
    uint8_t rc;
    uint8_t subscribe_channels[] =
        { 0x00, 0x01, 0x00, 0x10, 0x06, 0x01, 0x02, 0x0a,
        0x14, 0x15, 0x17, 0x01, 0x41
    };

#define TYPE_MSG_SUBSCRIBE 0x10

    rc = fbus_heartbeat();
    if (rc)
        return 96;

    return sendframe_wait(TYPE_MSG_SUBSCRIBE, subscribe_channels,
                          sizeof(subscribe_channels));
}

uint8_t fbus_heartbeat(void)
{
    uint8_t rc;
    uint8_t retry;
    uint8_t get_info[] = { 0x00, 0x01, 0x00, 0x07, 0x01, 0x00, 0x01, 0x60 };

    fbus_init();

    fbus_id_flag = 0;
    retry = 5;
    while (fbus_id_flag == 0 && --retry) {
        rc = sendframe_wait(TYPE_ID, get_info, sizeof(get_info));
        if (rc)
            return 97;

        _delay_ms_flag(5000, &fbus_id_flag);
    }

    if (fbus_id_flag == 0) {
        puts_P(PSTR("NOID!"));
    }

    return fbus_id_flag == 0;
}
