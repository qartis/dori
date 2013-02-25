#include <stdint.h>
#include <avr/interrupt.h>

#include "time.h"

volatile uint32_t time;

ISR(TIMER0_COMPA_vect)
{
    static uint8_t overflows = 0;

    overflows++;

    if (overflows >= 250) {
        overflows = 0;
        time++;
    }
}

void time_init(void)
{
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
