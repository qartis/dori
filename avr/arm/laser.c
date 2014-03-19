#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "laser.h"
#include "laser_defs.h"
#include "adc.h"
#include "uart.h"
#include "errno.h"

#define LASER_MAX_LINE_LEN 64

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

volatile uint16_t dist_mm;
volatile uint8_t error_flag;

volatile int8_t laser_alive;

inline uint8_t streq(const char *a, const char *b)
{
    return strcmp(a,b) == 0;
}

int strstart_P(const char *s1, const char * PROGMEM s2)
{
    return strncmp_P(s1, s2, strlen_P(s2)) == 0;
}

uint16_t parse_dist_str(const char *buf)
{
    char *comma;
    uint16_t dist;

    comma = strchr(buf, ',');
    if (comma == NULL)
        return 0;

    *comma = '\0';

    dist = atoi(buf + strlen("nDist: "));
    return dist;
}

ISR(USART_RX_vect)
{
    char buf[LASER_MAX_LINE_LEN];
    static uint8_t buf_len = 0;
    uint8_t c;

    c = UDR0;

    laser_alive = 1;

    if (dist_mm != 0)
        return;

    if (buf_len == LASER_MAX_LINE_LEN - 1) {
        /* we didn't handle this long line last time. let's drop it
           and start again */
        putchar('@');
        buf_len = 0;
    }

    buf[buf_len] = c;
    buf[buf_len + 1] = '\0';
    buf_len++;

    /* if the buffer contains a full line */
    if (c != '\n')
        return;

    if (strstart_P(buf, PSTR("OUT_RAN")) ||
            strstart_P(buf, PSTR("MEDIUM")) ||
            strstart_P(buf, PSTR("THICK"))) {
        printf("MEDIUM THICK\n");
        error_flag = 1;
        buf_len = 0;
        return;
    } else if (!strstart_P(buf, PSTR("nDist: "))) {
        buf_len = 0;
        return;
    }

    dist_mm = parse_dist_str(buf);
    buf_len = 0;
}

uint8_t has_power(void)
{
    uint8_t i;
    uint32_t v_ref;

#define SAMPLES 4

    v_ref = 0;

    for (i = 0; i < SAMPLES; i++) {
        v_ref += adc_read(6);
    }

    v_ref /= SAMPLES;

    return v_ref > 400;
}

void laser_off(void)
{
    uart_putchar('r');
    _delay_ms(5);
    uart_putchar('r');
    _delay_ms(5);
    uart_putchar('r');
    _delay_ms(5);
}

void laser_on(void)
{
    HOLD(ON_BTN);

    _delay_ms(50);

    if (has_power())
        puts_P(PSTR("laser_on ok"));
    else
        puts_P(PSTR("laser_on err"));
}

void laser_on_safe(void)
{
    laser_off();
    laser_on();
}

/* assumes laser is already on. return value 0 means error, because we need
   to represent the range 1mm .. 40000mm */
uint16_t measure_once(void)
{
    uint16_t retry;

    if (!has_power()) {
        laser_on();
    }

    dist_mm = 0;
    error_flag = 0;

    print_P(PSTR("*00004#"));

    /* 600 * 5 = 3000 ms */
    retry = 600;
    while (dist_mm == 0 && !error_flag && --retry)
        _delay_ms(5);

    uart_putchar('r');
    uart_putchar('r');
    uart_putchar('r');

    if (dist_mm != 0) {
        printf_P(PSTR("meas_once dist %d\n"), dist_mm);
        return dist_mm;
    } else if (error_flag) {
        puts_P(PSTR("meas_once err"));
        return 0;
    } else {
        puts_P(PSTR("meas_once timeout"));
        return 0;
    }
}

void laser_begin_rapidfire(void)
{
    if (!has_power()) {
        laser_on();
    }

    print_P(PSTR("*00084553#"));
}

uint8_t measure_rapid_fire(laser_callback_t cb)
{
    uint8_t rc;
    uint8_t read_flag;
    uint8_t retry;

    read_flag = 0;

    laser_begin_rapidfire();

    for (;;) {
        laser_alive = 0;
        dist_mm = 0;
        error_flag = 0;

        /* 255 * 10 = 2550 ms */
        retry = 255;
        while (dist_mm == 0 && !error_flag && --retry)
            _delay_ms(10);

        if (dist_mm == 0) {
            if (read_flag > 0) // heard some measurements
                break;

            if (laser_alive) // heard only errors
                printf_P(PSTR("LASER LIVE BUT NO MEASUREMENT - MOVE ME\n"));
            else
                printf_P(PSTR("HEARD NOTHING\n"));

            return ERR_LASER;
        }

        printf_P(PSTR("READ DIST: %d\n"), dist_mm);
        read_flag = 1;

        if (cb != NULL) {
            rc = cb(dist_mm);
            if (rc != 0) {
                return rc;
            }
        }
    }

    laser_off();

    return 0;
}

uint8_t laser_sweep(laser_callback_t cb)
{
    uint8_t rc;
    uint8_t seq_error_count;

    seq_error_count = 0;

    for (;;) {
        rc = measure_rapid_fire(cb);

        if (rc == 0) {
            seq_error_count = 0;
        } else if (rc == ERR_LASER) {
            /* maybe got a bunch of errors, maybe got just silence */
            seq_error_count++;

            if (seq_error_count > 2) {
                printf_P(PSTR("FAILED TO READ MEASUREMENTS\n"));
                break;
            }

            laser_off();
            laser_on();
        } else {
            break;
        }
    }

    laser_off();

    return rc;
}

void laser_init(void)
{
    /* ON button */
    /* OFF button */
    DDRD &= ~((1 << PORTD6) | (1 << PORTD3));

    laser_off();
}
