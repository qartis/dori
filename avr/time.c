#include <stdint.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include <stdio.h>

#include "time.h"
#include "irq.h"
#include "mcp2515.h"

#ifndef DEFAULT_PERIOD
#define DEFAULT_PERIOD 9000
#endif


#if F_CPU == 8000000L
/* 8000000 / 256 = 31250
    31250 / 125 = 250.
    so if we overflow every 125 ticks,
    then every 250 overflows is 1 second */
#define TIMER0_PRESCALER (1 << CS02) /* clk/256 */
#define OVERFLOW_TICKS 250

#elif F_CPU == 18432000L
/* 18432000 / 1024 = 18000
    18000 / 125 = 144 .
    so if we overflow every 125 ticks,
    then every 144 overflows is 1 second */
#define TIMER0_PRESCALER (1 << CS02) | (1 << CS00) /* clk/1024 */
#define OVERFLOW_TICKS 144

#else
#error no TIMER0 definition for clock speed
#endif

volatile uint32_t now;
volatile uint32_t uptime;
volatile uint32_t periodic_prev;
volatile uint16_t periodic_interval;
static volatile uint8_t overflows;

ISR(TIMER0_COMPA_vect)
{
    overflows++;

    if (overflows >= OVERFLOW_TICKS) {
        overflows = 0;
        uptime++;

        if (now > 0) {
            now++;
        }

        if (mcp2515_packet_time + 30 >= uptime) {
            wdt_reset();
        }

#ifdef SECONDS_IRQ
        irq_signal |= IRQ_SECONDS;
#endif

        if (now >= periodic_prev + periodic_interval) {
            irq_signal |= IRQ_PERIODIC;
            periodic_prev = now;
        }
    }
}

void time_init(void)
{
    periodic_interval = DEFAULT_PERIOD;

    /* timer0 is used as global timer for periodic messages */
    OCR0A = (125 - 1);
    OCR0B = (80 - 1);
    TCCR0A = (1 << WGM01);
    TCCR0B = TIMER0_PRESCALER;
    TIMSK0 = (1 << OCIE0A);
}

void time_set(uint32_t new_time)
{
    uint8_t run_periodic;

    run_periodic = 0;

    if (now != 0 && new_time > now
      && (new_time - now) > (periodic_prev + periodic_interval)) {
        run_periodic = 1;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        now = new_time;
        overflows = 0;
        TCNT0 = 0;
    }

    if (run_periodic) {
        irq_signal |= IRQ_PERIODIC;
    }
}
