#include <stdio.h>
#include <util/delay.h>

#include "i2c.h"
#include "nunchuck.h"
#include "uart.h"
#include "hmc5883.h"
 
int32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
    return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;
}

int main(void)
{
    struct nunchuck n;
    struct hmc5883 h;
    uint8_t rc;

    i2c_init(I2C_FREQ(400000));
    uart_init(BAUD(38400));

    while (nunchuck_init()){
        puts("nunchuck_init failed");
        _delay_ms(1000);
    }

    while (hmc_init()) {
        puts("hmc_init failed");
        _delay_ms(1000);
    }

    hmc_set_continuous();

    int16_t hxmin = -650, hxmax = 661;
    int16_t hymin = -987, hymax = 148;
    int16_t hzmin = -834, hzmax = 479;

    uint8_t min_x = 74, max_x = 186;
    uint8_t min_y = 67, max_y = 177;
    uint8_t min_z = 73, max_z = 182;

    for (;;) {
        puts("x ");

        rc = hmc_read(&h);
        if (rc) {
            puts("error reading");
            _delay_ms(100);
            hmc_init();
            hmc_set_continuous();
            continue;
        }

        rc = nunchuck_read(&n);
        if (rc) {
            puts("error reading");
            _delay_ms(100);
            nunchuck_init();
            continue;
        }

        prints16(h.x);
        putchar(' ');
        prints16(h.y);
        putchar(' ');
        prints16(h.z);
        putchar(' ');

        h.x = map(h.x, hxmin, hxmax, 1, 1000);
        h.y = map(h.y, hymin, hymax, 1, 1000);
        h.z = map(h.z, hzmin, hzmax, 1, 1000);
        h.x -= 500;
        h.y -= 500;
        h.z -= 500;

        n.accel_x = map(n.accel_x, min_x, max_x, 1, 100);
        n.accel_y = map(n.accel_y, min_y, max_y, 1, 100);
        n.accel_z = map(n.accel_z, min_z, max_z, 1, 100);
        int16_t accel_x = (int32_t)n.accel_x - 50;
        int16_t accel_y = (int32_t)n.accel_y - 50;
        int16_t accel_z = (int32_t)n.accel_z - 50;

        prints16(accel_x);
        putchar(' ');
        prints16(accel_y);
        putchar(' ');
        prints16(accel_z);
        putchar(' ');

        _delay_ms(30);
    }
 
    return 0;
}
