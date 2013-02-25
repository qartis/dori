#include <stdint.h>

#include "i2c.h"
#include "mpu6050.h"

#define MPU_ADDR 0xd0

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
