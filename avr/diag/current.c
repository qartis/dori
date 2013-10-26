#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "current.h"

uint16_t get_current(void)
{
    uint16_t val;

    val = adc_read(7);

    return val;
}
