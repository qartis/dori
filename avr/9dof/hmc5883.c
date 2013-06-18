#include <stdint.h>

#include "i2c.h"
#include "hmc5883.h"

#define HMC_ADDR 0x3c

#define HMC_REG_MODE   0x02
#define HMC_REG_OUTPUT 0x03
#define HMC_REG_IDENT  0x0a

#define HMC_MODE_SINGLE 0x01
#define HMC_MODE_CONTINUOUS 0x00

uint8_t hmc_init(void)
{
    uint8_t id[3];

    i2c_start(HMC_ADDR | I2C_WRITE);
    i2c_write(HMC_REG_IDENT);
    i2c_stop();

    i2c_start(HMC_ADDR | I2C_READ);
    id[0] = i2c_read(I2C_ACK);
    id[1] = i2c_read(I2C_ACK);
    id[2] = i2c_read(I2C_NOACK);
    i2c_stop();

    if (id[0] != 'H' || id[1] != '4' || id[2] != '3') {
        return 1;
    }

    return 0;
}

void hmc_set_continuous(void)
{
    i2c_start(HMC_ADDR | I2C_WRITE);
    i2c_write(HMC_REG_MODE);
    i2c_write(HMC_MODE_CONTINUOUS);
}

uint8_t hmc_read(struct hmc5883_t *h)
{
    int16_t x, y, z;
    uint8_t rc;

    rc = i2c_start(HMC_ADDR | I2C_WRITE);
    if (rc){
        return 1;
    }

    i2c_write(HMC_REG_OUTPUT);

    rc = i2c_start(HMC_ADDR | I2C_READ);
    if (rc){
        return 1;
    }

    x = i2c_read(I2C_ACK) << 8;
    x |= i2c_read(I2C_ACK);

    z = i2c_read(I2C_ACK) << 8;
    z |= i2c_read(I2C_ACK);

    y = i2c_read(I2C_ACK) << 8;
    y |= i2c_read(I2C_NOACK);

    i2c_stop();

    h->x = x;
    h->y = y;
    h->z = z;

    return 0;
}
