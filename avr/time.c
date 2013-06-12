#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "time.h"

volatile uint32_t now;
volatile uint32_t periodic_prev;
volatile uint16_t periodic_interval;

ISR(TIMER0_COMPA_vect)
{
    static uint8_t overflows = 0;

    overflows++;

    if (overflows >= 250) {
        overflows = 0;
        now++;
        //printf_P(PSTR("time %lu\n"), now);

        if (now >= periodic_prev + periodic_interval) {
            periodic_callback();
            periodic_prev = now;
        }
    }
}

void time_init(void)
{
    periodic_interval = 5;
    periodic_prev = 0;
    now = 0;

    /* timer0 is used as global timer for periodic messages */
    /* 8000000 / 256 = 31250
       31250 / 125 = 250.
       so if we overflow every 125 ticks,
       then every 250 overflows is 1 second */
    OCR0A = 125;
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS02); /* clk/256 */
    TIMSK0 = (1 << OCIE0A);
}

void time_set(uint32_t new_time)
{
    if (new_time > now
      && (new_time - now) > (periodic_prev + periodic_interval)) {
        periodic_callback();
    }

    now = new_time;
}
