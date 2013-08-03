#include <stdint.h>
#include <stdio.h>

#include "i2c.h"
#include "mpu6050.h"

#define MPU_ADDR 0xd0


uint8_t mpu_init(void)
{
    /* add error checking here: try to read chip's id */

    mpu_reg_write(MPU6050_RA_PWR_MGMT_1, 0x01);
    mpu_reg_write(MPU6050_RA_SMPLRT_DIV, 0x07);

    //printf("reg %d\n", mpu_reg_read(MPU6050_RA_PWR_MGMT_1));

    return 0;
}

uint8_t mpu_read(struct mpu6050_t *mpu)
{
    /* add error checking here */
    mpu->x = (int16_t)mpu_reg_read16(MPU6050_RA_GYRO_XOUT_H);
    mpu->y = (int16_t)mpu_reg_read16(MPU6050_RA_GYRO_YOUT_H);
    mpu->z = (int16_t)mpu_reg_read16(MPU6050_RA_GYRO_ZOUT_H);

    return 0;
}

void mpu_reg_write(uint8_t reg, uint8_t val)
{
    i2c_start(MPU_ADDR | I2C_WRITE);
    i2c_write(reg);
    i2c_write(val);
    i2c_stop();
}

uint8_t mpu_reg_read(uint8_t reg)
{
    uint8_t val;

    i2c_start(MPU_ADDR | I2C_WRITE);
    i2c_write(reg);

    i2c_start(MPU_ADDR | I2C_READ);
    val = i2c_read(I2C_NOACK);
    i2c_stop();

    return val;
}

uint16_t mpu_reg_read16(uint8_t reg)
{
    uint8_t valh, vall;

    i2c_start(MPU_ADDR | I2C_WRITE);
    i2c_write(reg);

    i2c_start(MPU_ADDR | I2C_READ);
    valh = i2c_read(I2C_ACK);
    vall = i2c_read(I2C_NOACK);
    i2c_stop();

    return (valh << 8) | vall;
}
