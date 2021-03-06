#include <stdio.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "ds18b20.h"
#include "onewire.h"

uint8_t temp_num_sensors;

void temp_init(void)
{
    temp_num_sensors = search_sensors();
    printf_P(PSTR("temp: [%d]\n"), temp_num_sensors);
}

uint8_t temp_begin(void)
{
    if (temp_num_sensors == 0) {
        temp_init();

        if (temp_num_sensors == 0) {
            return 1;
        }
    }

    if (ds18b20_start_meas(DS18B20_POWER_PARASITE, NULL)) {
        puts_P(PSTR("temp: er34"));
        temp_num_sensors = 0;
        return 1;
    }

    return 0;
}

void temp_wait(void)
{
    _delay_ms(DS18B20_TCONV_12BIT);
}

uint8_t temp_read(uint8_t channel, int16_t *temp)
{
    if (ds18b20_read_decicelsius(&sensor_ids[channel][0], temp)) {
        puts_P(PSTR("temp: crc!"));
        temp_num_sensors = 0;
        return 1;
    }

//    printf_P(PSTR("temp %d: %d\n"), channel, *temp);

    return 0;
}
