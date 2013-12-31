#include <stdint.h>

#include "adc.h"
#include "humidity.h"

uint16_t get_humidity(void)
{
    uint16_t val;

    val = adc_read(7);

    return val;
}
