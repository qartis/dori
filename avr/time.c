#include <stdint.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "time.h"
#include "irq.h"

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

ISR(TIMER0_COMPA_vect)
{
    overflows++;

    if (overflows >= 250) {
        overflows = 0;
        now++;

        /* heartbeat. delay should be moved out of ISR */
        PORTC &= ~(1 << PORTC3);
//        _delay_us(300);
        PORTC ^= (1 << PORTC3);

        if (now >= periodic_prev + periodic_interval) {
            periodic_tophalf();
            periodic_prev = now;
        }
    }
}

void time_init(void)
{
    periodic_interval = 9000;
    periodic_prev = 0;
    now = 0;

    /* timer0 is used as global timer for periodic messages */



#if F_CPU == 8000000L
    /* 8000000 / 256 = 31250
       31250 / 125 = 250.
       so if we overflow every 125 ticks,
       then every 250 overflows is 1 second */
    OCR0A = (125 - 1);
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS02); /* clk/256 */
    TIMSK0 = (1 << OCIE0A);
#elif F_CPU == 18432000L
    /* 18432000 / 1024 = 18000
       18000 / 125 = 144 .
       so if we overflow every 125 ticks,
       then every 144 overflows is 1 second */
    OCR0A = (144 - 1);
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS02) | (1 << CS00); /* clk/1024 */
    TIMSK0 = (1 << OCIE0A);
#else
#error no TIMER0 definition for baud rate
#endif

    /* heartbeat light */
    PORTC |= (1 << PORTC3);
    DDRC |= (1 << PORTC3);
}

void time_set(uint32_t new_time)
{
    if (new_time > now
      && (new_time - now) > (periodic_prev + periodic_interval)) {
        periodic_tophalf();
    }

    now = new_time;
    overflows = 0;
}
