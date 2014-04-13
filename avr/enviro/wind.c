#include <stdint.h>

#include "adc.h"
#include "wind.h"

uint16_t get_wind_speed(void)
{
    uint8_t i;
    uint32_t val;

#define NUM_SAMPLES 16

    val = 0;
    for (i = 0; i < NUM_SAMPLES; i++) {
        val += adc_read(6);
    }

    return val / NUM_SAMPLES;
}
