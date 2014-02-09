#include <stdint.h>

#include "adc.h"
#include "wind.h"

uint16_t get_wind_speed(void)
{
    uint16_t val;

    val = adc_read(6);

    return val;
}
