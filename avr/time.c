#include <stdint.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <stdio.h>

#include "time.h"
#include "irq.h"

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


#elif F_CPU == 1843200L
/* 1843200 / 256 = 17000
    17000 / 125 = 136 .
    so if we overflow every 125 ticks,
    then every 136 overflows is 1 second */
#define TIMER0_PRESCALER (1 << CS02) /* clk/256 */
#define OVERFLOW_TICKS 136


#else
#error no TIMER0 definition for baud rate
#endif


#define HEARTBEAT_OFF() PORTC |= (1 << PORTC3)
#define HEARTBEAT_ON()  PORTC &= ~(1 << PORTC3)

volatile uint32_t now;
volatile uint32_t periodic_prev;
volatile uint16_t periodic_interval;
static volatile uint8_t overflows;

void periodic_tophalf(void)
{
    if (irq_signal & IRQ_PERIODIC) {
        //puts_P(PSTR("timer!"));
    }

    irq_signal |= IRQ_PERIODIC;
}

ISR(TIMER0_COMPB_vect)
{
    overflows++;

    if (overflows >= OVERFLOW_TICKS) {
        HEARTBEAT_ON();
    }
}

ISR(TIMER0_COMPA_vect)
{
    if (overflows >= OVERFLOW_TICKS) {
        overflows = 0;
        now++;

        HEARTBEAT_OFF();

#ifdef TIMER_TOPHALF
        timer_tophalf();
#endif

        if (now >= periodic_prev + periodic_interval) {
            periodic_tophalf();
            periodic_prev = now;
        }
    }
}

void time_init(void)
{
    periodic_interval = DEFAULT_PERIOD;
    periodic_prev = 0;
    now = 0;

    /* timer0 is used as global timer for periodic messages */
    OCR0A = (125 - 1);
    OCR0B = (80 - 1);
    TCCR0A = (1 << WGM01);
    TCCR0B = TIMER0_PRESCALER;
    TIMSK0 = (1 << OCIE0A) | (1 << OCIE0B);

    HEARTBEAT_OFF();
//    DDRC |= (1 << PORTC3);
}

void time_set(uint32_t new_time)
{
    uint8_t run_periodic;

    run_periodic = 0;

    if (now != 0 && new_time > now
      && (new_time - now) > (periodic_prev + periodic_interval))
        run_periodic = 1;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        now = new_time;
        overflows = 0;
        TCNT0 = 0;
        PORTC &= ~(1 << PORTC3);
    }

    if (run_periodic)
        periodic_tophalf();
}
