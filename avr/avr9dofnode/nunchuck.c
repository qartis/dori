#include <avr/io.h>
#include <stdint.h>

#include "i2c.h"
#include "nunchuck.h"
 
#define NUNCHUCK_ADDR 0xa4

uint8_t nunchuck_init(void)
{
    if (i2c_start(NUNCHUCK_ADDR | I2C_WRITE)){
        return 1;
    }
    
    i2c_write(0xf0);
    i2c_write(0x55);
    i2c_write(0xfb);
    i2c_write(0x00);
    i2c_stop();

    return 0;
}

uint8_t nunchuck_read(struct nunchuck *n)
{
    uint8_t buf[6];
    uint8_t res;

    i2c_start(NUNCHUCK_ADDR | I2C_WRITE);
    i2c_write(0x00);
    i2c_stop();

    i2c_start(NUNCHUCK_ADDR | I2C_WRITE);
    i2c_write(0x00);
    i2c_stop();

    res = i2c_start(NUNCHUCK_ADDR | I2C_READ);
    if (res){
        return 1;
    }

    buf[0] = i2c_read(I2C_ACK);
    buf[1] = i2c_read(I2C_ACK);
    buf[2] = i2c_read(I2C_ACK);
    buf[3] = i2c_read(I2C_ACK);
    buf[4] = i2c_read(I2C_ACK);
    buf[5] = i2c_read(I2C_NOACK);

    n->joy_x = buf[0];
    n->joy_y = buf[1];

    n->z_button = !(buf[5] & (1 << 0));
    n->c_button = !(buf[5] & (1 << 1));

    /*
    n->accel_x = (buf[2]<<2);
    n->accel_y = (buf[3]<<2);
    n->accel_z = (buf[4]<<2);

    n->accel_x |= (buf[5] >> 2) & 0b00000011;
    n->accel_y |= (buf[5] >> 4) & 0b00000011;
    n->accel_z |= (buf[5] >> 6) & 0b00000011;
    */
    n->accel_x = buf[2];
    n->accel_y = buf[3];
    n->accel_z = buf[4];

    return 0;
}
