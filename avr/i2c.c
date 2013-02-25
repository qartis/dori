#include <stdint.h>
#include <util/twi.h>
#include <stdio.h>
 
#include "i2c.h"

/* Original Author: Peter Fleury <pfleury@gmx.ch> */
 
void i2c_init(uint8_t clk)
{
    TWSR = (0 << TWPS1) | (0 << TWPS0); /* no prescaler */
    TWBR = clk;
}
 
/* issue a start condition. zero return value means success */
uint8_t i2c_start(uint8_t address)
{
    uint8_t twst;
    uint8_t retry;
 
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
 
    retry = 255;
    while (! (TWCR & (1 << TWINT)) && --retry) {};
    if (!retry) {
        return 1;
    }

    twst = TW_STATUS & 0xF8;
    if ((twst != TW_START) && (twst != TW_REP_START)) {
        return 2;
    }
 
    TWDR = address;
    TWCR = (1 << TWINT) | (1 << TWEN);
 
    retry = 255;
    while (!(TWCR & (1 << TWINT)) && --retry) {};
    if (!retry){
        return 3;
    }
 
    twst = TW_STATUS & 0xF8;

    return ((twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK));
}
 
/* terminate the transfer */
void i2c_stop(void)
{
    uint8_t retry;

    /* send stop condition */
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
 
    /* wait until stop condition is executed and bus released */
    retry = 255;
    while ((TWCR & (1<<TWSTO)) && --retry) {};
}

/* returns 0 on success */
uint8_t i2c_write(uint8_t data)
{   
    uint8_t twst;
    uint8_t retry;

    /* send data to the previously addressed device */
    TWDR = data;
    TWCR = (1<<TWINT) | (1<<TWEN);
 
    /* wait until transmission completed */
    retry = 255;
    while (!(TWCR & (1<<TWINT)) && --retry) {};
 
    /* check status */
    twst = TW_STATUS & 0xF8;

    return (twst != TW_MT_DATA_ACK) || !retry;
}
 
uint8_t i2c_read(uint8_t ack)
{
    uint8_t retry;

    TWCR = (1 << TWINT) | (1 << TWEN) | (ack << TWEA);

    retry = 255;
    while (!(TWCR & (1 << TWINT)) && --retry) {};

    return TWDR;
}
