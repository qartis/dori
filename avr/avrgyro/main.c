#include <stdio.h>
#include <util/delay.h>
#include "i2c.h"
#include "uart.h"
#include "mpu6050.h"


void main(void)
{
    i2c_init(I2C_FREQ(400000));
    uart_init(BAUD(38400));

    puts("init");
    
    _delay_ms(500);

    mpu_reg_write(MPU6050_RA_PWR_MGMT_1, 0x01);
    mpu_reg_write(MPU6050_RA_SMPLRT_DIV, 0x07);

    for (;;) {
        int16_t val;
        val = (int16_t)mpu_reg_read16(MPU6050_RA_GYRO_ZOUT_H);

        print(val);
        putchar('\n');

        _delay_ms(5);
    }
}
