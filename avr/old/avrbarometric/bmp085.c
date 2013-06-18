#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <stdint.h>

#include "i2c.h"
#include "bmp085.h"
 
#define BMP085_ADDR 0xEE

#define BMP085_AC1_REG 0xAA
#define BMP085_AC2_REG 0xAC
#define BMP085_AC3_REG 0xAE
#define BMP085_AC4_REG 0xB0
#define BMP085_AC5_REG 0xB2
#define BMP085_AC6_REG 0xB4
#define BMP085_B1_REG 0xB6
#define BMP085_B2_REG 0xB8
#define BMP085_MB_REG 0xBA
#define BMP085_MC_REG 0xBC
#define BMP085_MD_REG 0xBE
#define BMP085_PARAM_MI 3791
#define BMP085_PARAM_MH -7357
#define BMP085_PARAM_MG 3038

#define OSS 3

int16_t ac1, ac2, ac3, b1, b2, mb, mc, md;
uint16_t ac4, ac5, ac6;

void bmp085_write_reg(uint8_t reg, uint8_t val)
{
    i2c_start(BMP085_ADDR | I2C_WRITE);
    i2c_write(reg);
    i2c_write(val);
    i2c_stop();
}

uint16_t bmp085_read_reg16(uint8_t reg)
{
    uint8_t msb, lsb;

    i2c_start(BMP085_ADDR | I2C_WRITE);
    i2c_write(reg);
    i2c_stop();

    i2c_start(BMP085_ADDR | I2C_READ);
    msb = i2c_read(I2C_ACK);
    lsb = i2c_read(I2C_NOACK);
    i2c_stop();

    return (msb << 8) | lsb;
}

uint8_t bmp085_read_reg8(uint8_t reg)
{
    uint8_t val;

    i2c_start(BMP085_ADDR | I2C_WRITE);
    i2c_write(reg);
    i2c_stop();

    i2c_start(BMP085_ADDR | I2C_READ);
    val = i2c_read(I2C_NOACK);
    i2c_stop();

    return val;
}

void bmp085_get_cal_param(void)
{
    ac1 = bmp085_read_reg16(BMP085_AC1_REG);
    ac2 = bmp085_read_reg16(BMP085_AC2_REG);
    ac3 = bmp085_read_reg16(BMP085_AC3_REG);
    ac4 = bmp085_read_reg16(BMP085_AC4_REG);
    ac5 = bmp085_read_reg16(BMP085_AC5_REG);
    ac6 = bmp085_read_reg16(BMP085_AC6_REG);
    b1  = bmp085_read_reg16(BMP085_B1_REG);
    b2  = bmp085_read_reg16(BMP085_B2_REG);
    mb  = bmp085_read_reg16(BMP085_MB_REG);
    mc  = bmp085_read_reg16(BMP085_MC_REG);
    md  = bmp085_read_reg16(BMP085_MD_REG);
}

struct bmp085_sample bmp085_read(void)
{
    struct bmp085_sample s;

    uint8_t msb, lsb, xlsb;
    int32_t ut;
    int32_t up;

    int32_t x1, x2, x3;
    int32_t b3, b5, b6;
    uint32_t b4, b7;
    int32_t t, p;

    /* read uncompensated temp value */
    bmp085_write_reg(0xF4, 0x2e);
    _delay_ms(5);
    ut = bmp085_read_reg16(0xf6);

    /* read uncompensated pressure value */
    bmp085_write_reg(0xf4, 0x34 + (OSS << 6));
    _delay_ms(30);
    msb = bmp085_read_reg8(0xf6);
    lsb = bmp085_read_reg8(0xf7);
    xlsb = bmp085_read_reg8(0xf8);

    up = ((int32_t)msb << 16) | ((int32_t)lsb << 8) | xlsb;
    up >>= (8 - OSS);


    /* calculate true temperature */
    x1 = ((ut - ac6) * ac5) >> 15;
    x2 = ((int32_t)mc << 11) / (x1 + md);
    b5 = x1 + x2;
    t = (b5 + 8) >> 4;


    /* calculate true pressure */
    b6 = b5 - 4000;
    x1 = ((int32_t)b2 * ((b6 * b6) >> 12)) >> 11;
    x2 = ((int32_t)ac2 * b6) >> 11;
    x3 = x1 + x2;
    b3 = ((((int32_t)ac1 * 4 + x3) << OSS) + 2) >> 2;
    x1 = ((int32_t)ac3 * b6) >> 13;
    x2 = ((int32_t)b1 * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = ((uint32_t)ac4 * (uint32_t)(x3 + 32768)) >> 15;
    b7 = ((uint32_t)up - b3) * ((uint32_t)50000UL >> OSS);
    if (b7 < 0x80000000L) {
        p = (b7 * 2) / b4;
    } else {
        p = (b7 / b4) * 2;
    }
    x1 = (p >> 8) * (p >> 8);
    x1 = (x1 * BMP085_PARAM_MG) >> 16;
    x2 = (p * BMP085_PARAM_MH) >> 16;
    p = p + ((x1 + x2 + BMP085_PARAM_MI) >> 4);

    s.temp = t;
    s.pressure = p;

    return s;
}
