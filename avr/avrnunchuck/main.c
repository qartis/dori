#include <stdio.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "i2c.h"
#include "nunchuck.h"
#include "uart.h"
 
int main(void)
{
    struct nunchuck n;

    i2c_init(I2C_FREQ(400000));
    uart_init(BAUD(38400));

    printf("system start\n");

    while (nunchuck_init()){
        printf("nunchuck failed to init\n");
        _delay_ms(500);
    }

    printf("nunchuck: ok\n");

    uint16_t x_max, x_min;
    uint16_t y_max, y_min;
    uint16_t z_max, z_min;

    x_max = y_max = z_max = 0;
    x_min = y_min = z_min = 0xffff;

    for (;;) {
        uint8_t res = nunchuck_read(&n);
        if (res){
            printf("error reading\n");
            _delay_ms(100);
            nunchuck_init();
            continue;
        }

        /*
        if (n.accel_x > x_max) {
            x_max = n.accel_x;
        } else if (n.accel_x < x_min) {
            x_min = n.accel_x;
        }
        if (n.accel_y > y_max) {
            y_max = n.accel_y;
        } else if (n.accel_y < y_min) {
            y_min = n.accel_y;
        }
        if (n.accel_z > z_max) {
            z_max = n.accel_z;
        } else if (n.accel_z < z_min) {
            z_min = n.accel_z;
        }
        */

        printf("%u\t%u\t", n.joy_x, n.joy_y);
        printf("%u\t%u\t%u\t", n.accel_x, n.accel_y, n.accel_z);
        printf("%c\t%c\n", n.z_button ? 'Z' : ' ', n.c_button ? 'C' : ' ');
        //printf("%u\t%u\t%u\t%u\t%u\t%u\n", x_min, x_max, y_min, y_max, z_min, z_max);

        _delay_ms(50);
    }
 
    return 0;
 
}
