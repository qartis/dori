#include <stdio.h>
#include <util/delay.h>

#include "ds18b20.h"
#include "onewire.h"

uint8_t num_sensors;

void temp_init(void)
{
    num_sensors = search_sensors();
    printf("%d sensors found\n", num_sensors);
}

uint8_t temp_begin(void)
{
    if (num_sensors == 0) {
        temp_init();

        if (num_sensors == 0) {
            return 1;
        }
    }

    if (ds18b20_start_meas(DS18B20_POWER_PARASITE, NULL)) {
        puts("short circuit?");
        num_sensors = 0;
        return 1;
    }

    return 0;
}

void temp_wait(void)
{
    _delay_ms(DS18B20_TCONV_12BIT);
}

void temp_read(uint8_t channel, int16_t *temp)
{
    if (ds18b20_read_decicelsius(&sensor_ids[channel][0], temp)) {
        puts("CRC Error? lost sensor?");
        num_sensors = 0;
        return;
    }

    printf("chan %d: %d\n", channel, *temp);
}
