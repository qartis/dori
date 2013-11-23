#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "timer.h"
#include "fbus.h"
#include "uart.h"

void print_len(const uint8_t *buf, uint8_t len)
{
    while (len--) {
        uart_putchar(*buf++);
    }
}

static uint8_t table[] = {
    /* ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet */
    '@',  0xa3, '$',  0xa5, 0xe8, 0xe9, 0xf9, 0xec,
    0xf2, 0xc7, '\n', 0xd8, 0xf8, '\r', 0xc5, 0xe5,
    '?',  '_',  '?',  '?',  '?',  '?',  '?',  '?',
    '?',  '?',  '?',  '?',  0xc6, 0xe6, 0xdf, 0xc9,
    ' ',  '!',  '\"', '#',  0xa4,  '%',  '&',  '\'',
    '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
    '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
    '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
    0xa1, 'A',  'B',  'C',  'D',  'E',  'F',  'G',
    'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
    'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
    'X',  'Y',  'Z',  0xc4, 0xd6, 0xd1, 0xdc, 0xa7,
    0xbf, 'a',  'b',  'c',  'd',  'e',  'f',  'g',
    'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
    'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
    'x',  'y',  'z',  0xe4, 0xf6, 0xf1, 0xfc, 0xe0
};

uint8_t bcd(uint8_t *dest, const char *s)
{
    uint8_t x, y, n, hi, lo;

    x = 0;
    y = 0;
    if (*s == '+'){
        s++;
    }
    while(s[x]) {
        lo = s[x++] - '0';
        if(s[x]){
            hi = s[x++] - '0';
        } else {
            hi = 0x0f;
        }

        n = (hi << 4) + lo;
        dest[y++] = n;
    }
    return y;
}

volatile char* addchar(volatile char *str, char c)
{
    *str = c;
    return str + 1;
}

volatile char phonenum_buf[16];
void unbcd_phonenum(volatile uint8_t *data)
{
    uint8_t len, n, x, at;
    volatile char *endptr = phonenum_buf;

    len = data[0];

    if(data[1] == 0x6f || data[1] == 0x91){
        endptr = addchar(endptr, '+');
    }

    at = 2;
    for(n = 0; n < len; ++n) {
        x = data[at] & 0x0f;
        if(x < 10){
            endptr = addchar(endptr, '0' + x);
        }
        n++;
        if(n >= len){
            break;
        }
        x = (data[at] >> 4) & 0x0f;
        if(x < 10){
            endptr = addchar(endptr, '0' + x);
        }
        at++;
    }
    *endptr = '\0';
}

uint8_t escaped(uint8_t c)
{
    switch (c){
    case 0x0a: return '\n';
    case 0x14: return '^';
    case 0x28: return '{';
    case 0x29: return '}';
    case 0x2f: return '\\';
    case 0x3c: return '[';
    case 0x3d: return '~';
    case 0x3e: return ']';
    case 0x40: return '|';
    default:   return '?';
    }
}

volatile char msg_buf[MSG_BUF_SIZE];
volatile uint8_t msg_buflen;
uint8_t unpack7_msg(volatile uint8_t *data, uint8_t len, volatile char* output)
{
    uint16_t *p, w;
    uint8_t c;
    uint8_t n;
    uint8_t shift = 0;
    uint8_t at = 0;
    uint8_t escape = 0;
    volatile char *endptr = output;

    for(n = 0; n < len; ++n) {
        p = (uint16_t *)(data + at);
        w = *p;
        w >>= shift;
        c = w & 0x7f;

        shift += 7;
        if(shift & 8) {
            shift &= 0x07;
            at++;
        }

        if (escape){
            endptr = addchar(endptr, escaped(c));
            escape = 0;
        } else if (c == 0x1b){
            escape = 1;
        } else {
            endptr = addchar(endptr, table[c]);
        }
    }
    *endptr = '\0';
    return len;
}

uint8_t gettrans(uint8_t c)
{
    uint8_t n;
    if (c == '?') return 0x3f;
    for (n = 0; n < 128; ++n){
        if (table[n] == c){
            return n;
        }
    }
    return 0x3f;
}

uint8_t pack7(uint8_t *dest, const char *s)
{
    uint8_t len;
    uint16_t *p, w;
    uint8_t at;
    uint8_t shift;
    uint8_t n, x;

    len = strlen(s);
    x = ((uint16_t)(len + 1) * 7) / 8;
    for(n = 0; n < x; ++n)
        dest[n] = 0;

    shift = 0;
    at = 0;
    w = 0;
    for(n = 0; n < len; ++n) {
        p = (uint16_t *)(dest + at);
        w = gettrans(s[n]) & 0x7f;
        w <<= shift;

        *p |= w;

        shift += 7;
        if(shift >= 8) {
            shift &= 7;
            at++;
        }
    }
    return x; // return packed length
}

void sendframe(uint8_t type, uint8_t *data, uint8_t size)
{
    // !!!! Don't use a buffer here
    // !!!! Compute the checksum on the fly instead
    uint8_t buf[256];
    uint8_t at, len, n;
    uint16_t check;
    uint16_t *p;

    at = 0;

    // build header
    buf[at++] = 0x1e;       // message startbyte
    buf[at++] = 0x00;       // dest: phone
    buf[at++] = 0x0c;       // source: PC
    buf[at++] = type;
    buf[at++] = 0x00;
    buf[at++] = size;

    // add data
    memcpy(buf+6, data, size);
    at += size;

    // if odd numbered, add filler byte
    if(size % 2) {
        buf[at++] = 0x00;
    }

    // calculate checksums
    check = 0;
    p = (uint16_t *)buf;
    len = at / 2;
    for(n = 0; n < len; ++n)
        check ^= p[n];
    p[n] = check;
    at += 2;

    // send the message!
    print_len(buf, at);
}

void sendack(uint8_t type, uint8_t seqnum)
{
    uint8_t buf[2];

    buf[0] = type;
    buf[1] = seqnum;

    sendframe(TYPE_ACK, buf, sizeof(buf)/sizeof(buf[0]));
}

void fbus_init(void)
{
    uint8_t c;
    for (c = 0; c < 128; c++){
        uart_putchar('U');
    }
    _delay_ms(5);
}

void uart_sendsms(const char *num, const char *ascii)
{
    uint8_t buf[256];
    uint8_t len = 0;
    buf[len++] = 0x00;
    buf[len++] = 0x01;
    buf[len++] = 0x00; //SMS frame header

    buf[len++] = 0x02;
    buf[len++] = 0x00;
    buf[len++] = 0x00; //send SMS message

    buf[len++] = 0x00;
    buf[len++] = 0x55;
    buf[len++] = 0x55;

    memset(buf+len, 0, 100);

    buf[len++] = 0x01;
    buf[len++] = 0x02;

    uint8_t packet_len_idx = len;
    buf[len++] = 0x00; // NUL, will be set later

    buf[len++] = 0x11; // hex field for sms properties
    buf[len++] = 0x00; // reference
    buf[len++] = 0x00; // PID
    buf[len++] = 0x00; // DCS
    buf[len++] = 0x00;
    buf[len++] = 0x04; // total blocks

    buf[len++] = 0x82;
    buf[len++] = 0x0c;
    buf[len++] = 0x01; // remote number
    buf[len++] = 0x07; // actual data length in this block

    buf[len++] = strlen(num); // length of phone number (unencoded)
    buf[len++] = 0x81;  // type - unknown

    len += bcd(buf + len, num);
    /*
    buf[len++] = 0x77;  // 778-896-9633
    buf[len++] = 0x88;
    buf[len++] = 0x69;
    buf[len++] = 0x69;
    buf[len++] = 0x33;
    */

    buf[len++] = 0x00; // start of SMSC block
    buf[len++] = 0x82;
    buf[len++] = 0x0c;
    buf[len++] = 0x02; // field type: SMSC number
    buf[len++] = 0x08;

    buf[len++] = 0x07; // start of SMSC number
    buf[len++] = 0x91; // type - international
    buf[len++] = 0x71; // 1-705-796-9300
    buf[len++] = 0x50;
    buf[len++] = 0x97;
    buf[len++] = 0x96;
    buf[len++] = 0x03;
    buf[len++] = 0xf0;

    buf[len++] = 0x80; // start of user data block

    uint8_t enc_len = pack7(buf + len + 3, ascii);
    uint8_t len_field_idx = len;

    buf[len++] = enc_len + 4; // "length + 4"
    buf[len++] = enc_len; // encoded length of sms msg
    buf[len++] = strlen(ascii); // un-encoded length of sms msg

    len += enc_len;

    if(buf[len_field_idx] % 8 != 0) {
        uint8_t pad_len = 8 - (buf[len_field_idx] % 8);
        uint8_t i;
        for(i = 0; i < pad_len; i++) {
            buf[len++] = 0x55;
        }

        buf[len_field_idx] += pad_len;
    }

    buf[len++] = 0x08;
    buf[len++] = 0x04;
    buf[len++] = 0x01;
    buf[len++] = 0xa9; // validity

    buf[packet_len_idx] = len - 9 - 1;

    buf[len++] = 0x01; // terminator
    buf[len++] = 0x41; // seq no

    sendframe(TYPE_SMS, buf, len);
    return;

    /*

    buf[len] = bcd(buf+len+2, "17057969300") + 1; //include the type-of-address
    buf[len+1] = 0x91;
    len += 12;

    buf[len++] = 0x11; //SMS Submit, Reject Duplicates, Validity Indicator present

    buf[len++] = 0x00; //message reference
    buf[len++] = 0x00; //protocol id
    buf[len++] = 0x00; //data coding scheme

    buf[len++] = strlen(ascii); //data len

    memset(buf+len, 0, 12);
    buf[len] = strlen(num);
    buf[len+1] = 0x91;
    bcd(buf+len+2, num);
    len += 12;

    buf[len++] = 0xa7; //validity period, 4 days
    buf[len++] = 0x00;
    buf[len++] = 0x00;
    buf[len++] = 0x00;
    buf[len++] = 0x00;
    buf[len++] = 0x00;
    buf[len++] = 0x00;


    buf[len++] = 0x01; //terminator
    buf[len++] = 0x41; //seq num

    if (len % 2){
        buf[len] = 0x00; //padding, but don't increment len
    }

    sendframe(TYPE_SMS, buf, len);
    */
}

void fbus_sendsms(const char *num, const char *msg)
{
    fbus_init();
    uart_sendsms(num, msg);
}

/*
uint8_t fbus_sendsms(const char *num, const char *msg)
{
    fbus_init();
    uart_sendsms(num, msg);
    timer_start();
    for(;;){
        if (frametype == FRAME_SMS_SENT){
            timer_disable();
            return 1;
        } else if (frametype == FRAME_READ_TIMEOUT){
            timer_disable();
            return 0;
        }
        //TODO what if it's an important packet?
    }
}
*/

#if 0
unsigned char req[] = {FBUS_FRAME_HEADER, 0x04,
0x02, /* 0x01 for SM, 0x02 for ME */
0x00, /* folder_id */
0x00, /* Location Hi */
0x00, /* Location Lo */
0x0F, 0x55};
void fbus_delete_sms(uint8_t memory_type, uint8_t storage_loc)
{
    uint8_t del_sms[] = {0x00, 0x01, 0x00, 0x0a, memory_type, storage_loc, 0x01};

    fbus_init();
    sendframe(TYPE_SMS_MGMT, del_sms, sizeof(del_sms));
}
#endif

void fbus_subscribe(void)
{
#define TYPE_MSG_SUBSCRIBE 0x10
    uint8_t subscribe_channels[] = 
        {0x00, 0x01, 0x00, 0x10, 0x06, 0x01, 0x02, 0x0a, 
         0x14, 0x15, 0x17, 0x01, 0x41};

    fbus_init();
    sendframe(TYPE_MSG_SUBSCRIBE, subscribe_channels,
            sizeof(subscribe_channels));
}

enum fbus_frametype fbus_heartbeat(void)
{
    uint8_t get_info[] = 
        {0x00, 0x01, 0x00, 0x07, 0x01, 0x00, 0x01, 0x60};

    fbus_init();
    sendframe(TYPE_GETID, get_info,
            sizeof(get_info));
    return FRAME_READ_TIMEOUT;
}
