#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <util/atomic.h>

#define UART_BAUD 38400
#define DEBUG

#include "irq.h"
#include "debug.h"
#include "time.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "node.h"
#include "can.h"
#include "laser.h"

#define MIN(a,b) a < b ? a : b

inline uint8_t streq(const char *a, const char *b)
{
    return strcmp(a,b) == 0;
}

volatile uint16_t dist_mm;
volatile uint8_t read_flag;

volatile int8_t laser_alive;

int strstart_P(const char *s1, const char * PROGMEM s2)
{
    return strncmp_P(s1, s2, strlen_P(s2)) == 0;
}

uint16_t parse_dist_str(const char *buf)
{
    char *comma;
    uint16_t dist;

    if (!strstart_P(buf, PSTR("Dist: ")))
        return -1;

    comma = strchr(buf, ',');
    if (comma == NULL)
        return -1;

    *comma = '\0';

    dist = atoi(buf + strlen("Dist: "));
    return dist;
}

extern volatile uint8_t uart_ring[UART_BUF_SIZE];
extern volatile uint8_t ring_in;
extern volatile uint8_t ring_out;

#define UDR UDR0

ISR(USART_RX_vect)
{
    uint8_t data = UDR;

    laser_alive = 1;

    if(read_flag)
        return;

    if (data == '\r')
        return;

    uart_ring[ring_in] = data;
    ring_in = (ring_in + 1) % UART_BUF_SIZE;
    uart_ring[ring_in] = '\0';

    /* if the buffer contains a full line */
    if (data != '\n') {
        return;
    }


    if(ring_in < strlen("Dist: ") || !strstart_P((const char*)uart_ring, PSTR("Dist: "))) {
        ring_in = ring_out = 0;
        return;
    }

    dist_mm = parse_dist_str((const char*)uart_ring);
    ring_in = ring_out = 0;
    read_flag = 1;
}


void adc_init(void)
{
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel)
{
    ADMUX = (1 << REFS0) | channel; /* internal vref */

    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) {};

    uint8_t low = ADCL;
    uint8_t high = ADCH;
    return ((uint16_t)(high<<8)) | low;
}

uint8_t has_power(void)
{
    uint16_t v_ref = adc_read(7);
    return v_ref > 400;
}

void turn_off(void)
{
    // turn off the laser in case it was in the frozen state
    uart_putchar('r');
    _delay_ms(50);
    uart_putchar('r');
    _delay_ms(50);
    uart_putchar('r');
    _delay_ms(50);


    // cancel any mode the laser is in
    PRESS(OFF_BTN);

    // turn the device off if it is on
    HOLD(OFF_BTN);

    // if the laser is on
    if(has_power()) {
        printf_P(PSTR("DEVICE FAILED TO TURN OFF\n"));
    }
    else {
        printf_P(PSTR("DEVICE TURNED OFF\n"));
    }
}

void turn_on(void)
{
    // turn on the device
    HOLD(ON_BTN);

    // if the laser is on
    if(has_power()) {
        printf_P(PSTR("DEVICE TURNED ON\n"));
    }
    else {
        printf_P(PSTR("DEVICE FAILED TO TURN ON\n"));
    }
}

void turn_on_safe(void)
{
    turn_off();
    // turn on the device
    turn_on();
}

// assumes laser is already on
void measure_once(void)
{
    read_flag = 0;
    ring_out = ring_in = 0;

    PRESS(ON_BTN);

    PRESS(ON_BTN);

    uint16_t retry = 16384;// 0xFFFF;
    while(--retry && !read_flag)
    {
        _delay_us(300);
    }

    if(read_flag) {
        printf_P(PSTR("LASER DIST %d\n"), dist_mm);
    }
    else {
        printf_P(PSTR("LASER FAIL TO MEASURE\n"));
    }

    // cancel any mode the laser got into
    PRESS(OFF_BTN);
}


// reads up to 100 measurements
int8_t measure_rapid_fire(uint8_t target_count)
{
    uint8_t read_count;
    uint16_t retry;

    read_flag = 0;
    ring_out = ring_in = 0;

    if(!has_power()) {
        printf_P(PSTR("no power\n"));
        return -2;
    }

    // go into rapid fire mode
    HOLD(ON_BTN);

    read_count = 0;
    while(read_count < target_count) {
        laser_alive = 0;
        read_flag = 0;

        retry = 3333; // 300us * 3333 = ~1 second
        while(!read_flag && --retry) {
            _delay_us(300);
        }

        if(read_flag) {
            printf_P(PSTR("READ DIST: %d\n"), dist_mm);
            read_count++;
            continue;
        }


        if(read_count > 0) {
            // heard some measurements
            break;
        }
        else if(laser_alive) {
            // heard only errors
            return -1;
        }
        else {
            // didn't hear anything this whole time
            return -2;
        }
    }

    // cancel rapid fire mode
    PRESS(OFF_BTN);

    return read_count;
}

// assumes laser is already on
void measure(uint32_t target_count)
{
    printf_P(PSTR("%d READINGS\n"), target_count);
    _delay_ms(100);

    uint32_t remaining = target_count;
    uint8_t num_to_read;
    int8_t read;

    // sequential error count
    uint8_t seq_error_count = 0;

    while(remaining > 0) {
        num_to_read = MIN(remaining, 100);
        read = measure_rapid_fire(num_to_read);

        // what about if read = 0?
        if(read > 0) {
            // no errors
            remaining -= read;
            seq_error_count = 0;
        }
        else if(read == -1) {
            // timed out, heard no measurements
            // but the device is still talking
            printf_P(PSTR("LASER LIVE BUT NO MEASUREMENT - MOVE ME\n"));
            seq_error_count++;
        }
        else if(read == -2) {
            // timed out and nothing heard from the device
            printf_P(PSTR("HEARD NOTHING\n"));
            seq_error_count++;

            if(seq_error_count == 1) {
                // reset device to default state
                printf_P(PSTR("BACK TO DEFAULT MENU\n"));
                PRESS(OFF_BTN);
            }
            else if(seq_error_count == 2) {
                printf_P(PSTR("RESTARTING\n"));
                turn_on_safe();
            }
            else {
                printf_P(PSTR("FAILED TO READ MEASUREMENTS\n"));
                PRESS(OFF_BTN);
                break;
            }
        }
    }

    printf_P(PSTR("TOOK %d READINGS\n"), target_count - remaining);
}

uint8_t debug_irq(void)
{
    char buf[64];
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf)-1] = '\0';

    if(streq(buf, "on")) {
        turn_on();
    } else if(streq(buf, "son")) {
        turn_on_safe();
    } else if(streq(buf, "m")) {
        measure_once();
    } else if(buf[0] == 'r') {
        unsigned int max_count = 20;

        if(strlen(buf) > 2) {
            max_count = atoi(&buf[2]);
        }
        measure(max_count);
    } else if(streq(buf, "off")) {
        turn_off();
    } else if(streq(buf, "c")) {
        PRESS(ON_BTN);
    } else {
        printf_P(PSTR("err: '%s'\n"), buf);
    }

    debug_flush();

    _delay_ms(500);
    PROMPT;

    return 0;
}

uint8_t can_irq(void)
{
    uint8_t buf[2];

    switch (packet.type) {
    case TYPE_value_request:
        turn_on_safe();
        measure_once();
        turn_off();

        if(read_flag)
        {
            buf[0] = dist_mm >> 8;
            buf[1] = (dist_mm & 0x00FF);
            mcp2515_send_sensor(TYPE_value_explicit,
                                packet.id,
                                buf,
                                2,
                                SENSOR_laser);

        }
        else {
            mcp2515_send_sensor(TYPE_sensor_error,
                                packet.id,
                                buf,
                                2,
                                SENSOR_laser);
        }

        read_flag = 0;
    }

    packet.unread = 0;
    return 0;
}

uint8_t uart_irq(void)
{
    return 0;
}

uint8_t periodic_irq(void)
{
    return 0;
}

void input_init(void)
{
    // make ON button input pin
    DDRD &= ~(1 << PIND5);

    // make OFF button input pin
    DDRD &= ~(1 << PIND6);

    // make V+ input pin
    DDRD &= ~(1 << PIND7);

    // no pullup
    PORTD = 0;
}

void main(void)
{
    input_init();
    adc_init();

    NODE_INIT();
    sei();

    NODE_MAIN();
}
