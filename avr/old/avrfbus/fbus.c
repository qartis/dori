#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>

#include "timer.h"
#include "fbus.h"
#include "power.h"
#include "../uart.h"

#define TYPE_SMS_MGMT 0x14
#define TYPE_SMS 0x02
#define TYPE_ACK 0x7f
#define TYPE_GETID 0xd1
#define TYPE_ID  0xd2
#define TYPE_NET_STATUS 0x0a

inline void print_len(const uint8_t *buf, uint8_t len){
    while(len--){
        uart_putchar(*buf++);
    }
}

uint8_t table[] = {
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

uint8_t bcd(uint8_t *dest, const char *s){
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

char* addchar(char *str, char c){
    *str = c;
    return str + 1;
}

char phonenum_buf[16];
void unbcd_phonenum(uint8_t *data){
    uint8_t len, n, x, at;
    char *endptr = phonenum_buf;

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

uint8_t escaped(uint8_t c){
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

char msg_buf[32];
void unpack7_msg(uint8_t *data, uint8_t len){
    uint16_t *p, w;
    uint8_t c;
    uint8_t n;
    uint8_t shift = 0;
    uint8_t at = 0;
    uint8_t escape = 0;
    char *endptr = msg_buf;

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
}

uint8_t gettrans(uint8_t c){
    uint8_t n;
    if (c == '?') return 0x3f;
    for (n = 0; n < 128; ++n){
        if (table[n] == c){
            return n;
        }
    }
    return 0x3f;
}

uint8_t pack7(uint8_t *dest, const char *s){
    uint8_t len;
    uint16_t *p, w;
    uint8_t at;
    uint8_t shift;
    uint8_t n, x;

    len = strlen(s);
    x = ((uint16_t)len * 8) / 7;
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
    return len;
}

void sendframe(uint8_t type, uint8_t *data, uint8_t size){
    static uint8_t buf[128];
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

void sendack(uint8_t type, uint8_t seqnum){
    uint8_t buf[2];

    buf[0] = type;
    buf[1] = seqnum;

    sendframe(TYPE_ACK, buf, sizeof(buf)/sizeof(buf[0]));
}

enum fbus_frametype fbus_readframe(uint8_t timeout){
    static uint8_t buf[128];
    int8_t n;
    
retry:
    n = read_timeout(buf, 1, timeout);
    if (n == -1){
        return FRAME_READ_TIMEOUT;
    }
    if (buf[0] != 0x1e){
        goto retry;
    }
    n = read_timeout(buf, 1, timeout);
    if (n == -1){
        return FRAME_READ_TIMEOUT;
    }
    if (buf[0] != 0x0c){
        goto retry;
    }
    n = read_timeout(buf, 1, timeout);
    if (n == -1){
        return FRAME_READ_TIMEOUT;
    }
    if (buf[0] != 0x00){
        goto retry;
    }
    n = read_timeout(buf, 3, timeout);
    if (n == -1){
        return FRAME_READ_TIMEOUT;
    }
    uint8_t type = buf[0];
    uint8_t len = buf[2];
    uint8_t padding = len%2;
    if (len > 128){
        return FRAME_UNKNOWN;
    }
    n = read_timeout(buf, len + padding, timeout);
    if (n == -1){
        return FRAME_READ_TIMEOUT;
    }
    uint8_t seq_no = buf[n-1-padding];
    uint8_t ignored_checksum[2];
    n = read_timeout(ignored_checksum, 2, timeout);
    if (n == -1){
        return FRAME_READ_TIMEOUT;
    }
    if (type == TYPE_ACK){
        goto retry;
    }
    sendack(type, seq_no & 0x0f);
    delay_ms(100);
    if (type == TYPE_SMS && buf[3] == 0x10){
        unbcd_phonenum(buf+23);
        unpack7_msg(buf+42, buf[22]);
        fbus_delete_sms(buf[4], buf[5]);
        return FRAME_SMS_RECV;
    } else if (type == TYPE_SMS && buf[3] == 0x02){
        return FRAME_SMS_SENT;
    } else if (type == TYPE_SMS && buf[3] == 0x03){
        return FRAME_SMS_ERROR;
    } else if (type == TYPE_ID){
        return FRAME_ID;
    } else if (type == TYPE_NET_STATUS){
        return FRAME_NET_STATUS;
    } else {
        return FRAME_UNKNOWN;
    }
}

void uart_sendsms(const char *num, const char *ascii){
    static uint8_t buf[64];
    uint8_t len = 0;
    buf[len++] = 0x00;
    buf[len++] = 0x01;
    buf[len++] = 0x00; //SMS frame header

    buf[len++] = 0x01;
    buf[len++] = 0x02;
    buf[len++] = 0x00; //send SMS message

    memset(buf+len, 0, 12);
    buf[len] = bcd(buf+len+2, "8613800571500") + 1; //include the type-of-address
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

    len += pack7(buf + len, ascii);

    buf[len++] = 0x01; //terminator
    buf[len++] = 0x41; //seq num

    if (len % 2){
        buf[len] = 0x00; //padding, but don't increment len
    }

    sendframe(TYPE_SMS, buf, len);
}

void fbus_init(void){
    uint8_t c;
    for (c = 0; c < 128; c++){
        uart_putchar('U');
        delay_ms(1);
    }
    delay_ms(1);
}

uint8_t fbus_sendsms(const char *num, const char *msg){
    enum fbus_frametype f;

    fbus_init();
    uart_sendsms(num, msg);
    timer_start();
    for(;;){
        f = fbus_readframe(10);
        if (f == FRAME_SMS_SENT){
            timer_disable();
            return 1;
        } else if (f == FRAME_READ_TIMEOUT){
            timer_disable();
            return 0;
        }
        //TODO what if it's an important packet?
    }
}

void fbus_delete_sms(uint8_t memory_type, uint8_t storage_loc){
    uint8_t del_sms[] = {0x00, 0x01, 0x00, 0x0a, memory_type, storage_loc, 0x01};

    fbus_init();
    sendframe(TYPE_SMS_MGMT, del_sms, sizeof(del_sms));
}

enum fbus_frametype fbus_heartbeat(void){
    uint8_t get_info[] = {0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x60};
    enum fbus_frametype type;

    for(;;){
        fbus_init();
        sendframe(TYPE_GETID, get_info, sizeof(get_info));
        timer_start();
        type = fbus_readframe(1);
        if (type != FRAME_READ_TIMEOUT){
            timer_disable();
            return type;
        }
        power_press_release();
        delay_ms(10000);
    }
}
