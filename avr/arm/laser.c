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

#define LASER_MAX_LINE_LEN 64

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

volatile uint16_t dist_mm;
volatile uint8_t has_new_dist;
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
        return -1;

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

    if (has_new_dist)
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

    if (strstart_P(buf, PSTR("OUT_RAN"))) {
        printf("OUTRAN\n");
        error_flag = 1;
        buf_len = 0;
        return;
    } else if (strstart_P(buf, PSTR("MEDIUM")) ||
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
    has_new_dist = 1;
}

uint8_t has_power(void)
{
    uint8_t i;
    uint32_t v_ref;

    v_ref = 0;

    for (i = 0; i < 16; i++) {
        v_ref += adc_read(6);
    }

    v_ref /= 16;

    return v_ref > 400;
}

void laser_off(void)
{
    // turn off the laser in case it was in the frozen state
    uart_putchar('r');
    _delay_ms(5);
    uart_putchar('r');
    _delay_ms(5);
    uart_putchar('r');
    _delay_ms(5);
}

void laser_on(void)
{
    if (has_power()) {
        puts_P(PSTR("laser_on device already has power"));
        return;
    }

    HOLD(ON_BTN);

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
    uint8_t retry;

    if (!has_power()) {
        laser_on();
    }

    has_new_dist = 0;
    error_flag = 0;

    print_P(PSTR("*00004#"));

    /* 255 * 10 = 2550 ms */
    retry = 255;
    while (!has_new_dist && !error_flag && --retry)
        _delay_ms(10);

    _delay_ms(100);
    uart_putchar('r');
    uart_putchar('r');
    uart_putchar('r');

    if (has_new_dist) {
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

// reads up to 100 measurements
int8_t measure_rapid_fire(uint8_t target_count)
{
    uint8_t read_count;
    uint8_t retry;

    has_new_dist = 0;

    if (!has_power()) {
        laser_on();
    }

    print_P(PSTR("*00084553#"));

    read_count = 0;
    while (read_count < target_count) {
        laser_alive = 0;
        has_new_dist = 0;
        error_flag = 0;

        /* 255 * 5 = 1275 ms */
        retry = 255;
        while(!has_new_dist && !error_flag && --retry)
            _delay_ms(10);

        if (has_new_dist) {
            printf_P(PSTR("READ DIST: %d\n"), dist_mm);
            read_count++;
            continue;
        }

        if (read_count > 0) // heard some measurements
            break;
        else if (laser_alive) // heard only errors
            return -1;
        else // didn't hear anything this whole time
            return -2;
    }

    uart_putchar('r');
    uart_putchar('r');
    uart_putchar('r');

    return read_count;
}

// assumes laser is already on
void measure(uint32_t target_count)
{
    uint32_t remaining;
    uint8_t num_to_read;
    int8_t read;

    // sequential error count
    uint8_t seq_error_count = 0;

    printf_P(PSTR("%d READINGS\n"), target_count);
    _delay_ms(100);

    remaining = target_count;
    while (remaining > 0) {
        num_to_read = MIN(remaining, 100);
        read = measure_rapid_fire(num_to_read);

        // what about if read = 0?
        if (read > 0) {
            // no errors
            remaining -= read;
            seq_error_count = 0;
        } else if (read == -1) {
            // timed out, heard no measurements
            // but the device is still talking
            printf_P(PSTR("LASER LIVE BUT NO MEASUREMENT - MOVE ME\n"));
            seq_error_count++;
        } else if (read == -2) {
            // timed out and nothing heard from the device
            printf_P(PSTR("HEARD NOTHING\n"));
            seq_error_count++;
        }

        if (seq_error_count == 1) {
            // reset device to default state
            printf_P(PSTR("BACK TO DEFAULT MENU\n"));
            PRESS(OFF_BTN);
        } else if (seq_error_count == 2) {
            printf_P(PSTR("RESTARTING\n"));
            laser_on_safe();
        } else if (seq_error_count > 2) {
            printf_P(PSTR("FAILED TO READ MEASUREMENTS\n"));
            PRESS(OFF_BTN);
            break;
        }
    }

    printf_P(PSTR("TOOK %d READINGS\n"), target_count - remaining);
}

void laser_init(void)
{
    /* ON button */
    DDRD &= ~(1 << PORTD6);

    /* OFF button */
    DDRD &= ~(1 << PORTD3);
}
