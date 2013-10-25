#include <stdio.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "i2c.h"
#include "bmp085.h"
#include "uart.h"
#include "wind.h"
#include "humidity.h"
#include "temp.h"

volatile uint8_t water_tips;

ISR(PCINT2_vect)
{
    if (PIND & (1 << PORTD6)) {
        water_tips++;
        _delay_us(500);
    }
}
 
int main(void)
{
    struct bmp085_sample s[20] = {{0}};
    int32_t sum = 0;
    uint8_t i = 0;

    i2c_init(I2C_FREQ(400000));
    uart_init(BAUD(38400));

    adc_init();

    PCMSK2 |= (1 << PCINT22);
    PCICR |= (1 << PCIE2);
    PORTD |= (1 << PORTD6);

    sei();

    temp_init();
    
    bmp085_get_cal_param();

    water_tips = 0;

    for (;;) {
        if (num_sensors == 0) {
            printf("no sensors\n");
            temp_init();
        }

        temp_begin();
        temp_wait();

        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            printf("tips  %d\n", water_tips);
            water_tips = 0;
        }

        int16_t temp;
        for (i = 0; i < num_sensors; i++) {
            temp_read(i, &temp);
        }

        printf("wind %d\n", get_wind_speed());
        printf("humi %d\n", get_humidity());
        printf("baro %ld\n", bmp085_read());
        _delay_ms(500);
    }
 
    return 0;
 
}
