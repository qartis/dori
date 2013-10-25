#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "humidity.h"

uint16_t get_humidity(void)
{
    uint16_t val;

    val = adc_read(7);

    return val;
}
