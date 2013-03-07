#include <stdio.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "i2c.h"
#include "bmp085.h"
#include "uart.h"
 
int main(void)
{
    struct bmp085_sample s[20] = {{0}};
    int32_t sum = 0;
    uint8_t i = 0;

    i2c_init(I2C_FREQ(400000));
    uart_init(BAUD(38400));
    
    bmp085_get_cal_param();

    for (;;) {
        sum -= s[i].pressure;
        s[i] = bmp085_read();
        sum += s[i].pressure;


        printf("%ld %ld\n", s[i].temp, sum / 20);
        i++;
        i %= 20;
        _delay_ms(500);
    }
 
    return 0;
 
}
