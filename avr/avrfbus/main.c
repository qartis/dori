#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include "led.h"
#include "nmea.h"
#include "timer.h"
#include "fbus.h"
#include "uart.h"
#include "ts555.h"
#include "multiplex.h"
#include "power.h"

#define GPS_BAUD 9600
#define PHONE_BAUD 115200

uint8_t streq(const char *a, const char *b){
    return strcmp(a,b) == 0;
}

inline uint8_t strstart(const char *buf1, const char *buf2){
    return strncmp(buf1,buf2,strlen(buf2)) == 0;
}

volatile uint8_t should_ping;

EMPTY_INTERRUPT(PCINT2_vect);

ISR(PCINT0_vect){
    if (ts555_active){
        should_ping = 1;
    }
}

void handle_fbus_packet(enum fbus_frametype type){
    switch (type){
    case FRAME_READ_TIMEOUT:
    case FRAME_NET_STATUS:
    case FRAME_ID:
        break;
    case FRAME_SMS_RECV:
        printf("from '%s'\nmsg: '%s'\n", msg_buf, phonenum_buf);
        if (streq(msg_buf, "ping")){
            uint8_t rc = fbus_sendsms(phonenum_buf, "pong");
            printf("sms: %u\n", rc);
        }
        break;
    default:
        printf("strange type %u\n", type);
        break;
    }
}

int main(void) {
    wdt_disable();
    led_init();
    uart_init(BAUD(PHONE_BAUD));
    multiplex_init();
    ts555_init();
    power_init();
    sei();
    delay_ms(10);

    PCMSK2 = 1<<PCINT16;
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    for(;;){
        should_ping = 0;
        ts555_trigger();
            PCICR |=  (1<<PCIE0);
            PCICR |=  (1<<PCIE2);
                led_off();
                    sleep_mode();
                led_on();
            PCICR &= ~(1<<PCIE2);
            PCICR &= ~(1<<PCIE0);
        ts555_deactivate();

        enum fbus_frametype type;
        do {
            timer_start();
            type = fbus_readframe(2);
            timer_disable();
            handle_fbus_packet(type);
        } while (type != FRAME_READ_TIMEOUT);

        if (should_ping){
            should_ping = 0;
            type = fbus_heartbeat();
            handle_fbus_packet(type);
        }
    }
}
