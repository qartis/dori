#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "uart.h"
#include "onewire.h"
#include "ds18x20.h"

int main(void)
{
	uint8_t nSensors, i;
	int16_t decicelsius;
    //char s[10];
	
	uart_init(BAUD(9600));
	
	sei();
	
restart:
	nSensors = search_sensors();
	//printf("%d sensors found\n",nSensors);

    if (nSensors == 0) {
        _delay_ms(500);
        goto restart;
    }

	for (;;) {
		if (DS18X20_start_meas(DS18X20_POWER_PARASITE, NULL)) {
            //printf("short circuit?\n");
            goto restart;
        }

        _delay_ms(DS18B20_TCONV_12BIT);
        for (i = 0; i < nSensors; i++) {
            if (DS18X20_read_decicelsius(&sensor_ids[i][0], &decicelsius)) {
    //            printf("CRC Error\n");
                goto restart;
            }

            print(decicelsius);
            uart_putchar('\n');
            //print_temp(decicelsius, s, 10);
            //printf("%s\n",s);
        }
        uart_putchar('\n');
        //printf("\n");
	}
}
