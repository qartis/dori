#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "uart.h"
#include "i2c.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "node.h"
#include "hmc5883.h"
#include "mpu6050.h"
#include "nunchuck.h"
#include "nmea.h"
#include "irq.h"
#include "can.h"

#define streq_P(a,b) (strcmp_P(a,b) == 0)

uint8_t uart_irq(void)
{
    char buf[UART_BUF_SIZE];

    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';

    if (parse_nmea(buf))
        return 0;

    if (streq_P(buf, "read")) {
        puts_P(PSTR("read"));
    }

    return 0;
}

uint8_t periodic_irq(void)
{
    uint8_t rc;
    struct hmc5883_t hmc;
    struct mpu6050_t mpu;
    struct nunchuck_t nunchuck;
    struct mcp2515_packet_t p;

    hmc.x = -1;
    hmc.y = -1;
    hmc.z = -1;

    mpu.x = -1;
    mpu.y = -1;
    mpu.z = -1;

    nunchuck.accel_x = 255;
    nunchuck.accel_y = 255;
    nunchuck.accel_z = 255;

    rc = hmc_read(&hmc);
    if (rc) {
        /* sensor error, reinit */
        return rc;
    }

    rc = mpu_read(&mpu);
    if (rc) {
        /* sensor error, reinit */
        return rc;
    }

    rc = nunchuck_read(&nunchuck);
    if (rc) {
        /* sensor error, reinit */
        return rc;
    }

    p.type = TYPE_value_periodic;
    p.id = ID_gyro;
    p.data[0] = hmc.x >> 8;
    p.data[1] = hmc.x;
    p.data[2] = hmc.y >> 8;
    p.data[3] = hmc.y;
    p.data[4] = hmc.z >> 8;
    p.data[5] = hmc.z;
    p.len = 6;
    rc = mcp2515_send2(&p);
    if (rc) {
        /* can error, attempt to transmit error, reinit */
        return rc;
    }

    _delay_ms(150);
    p.type = TYPE_value_periodic;
    p.id = ID_compass;
    p.data[0] = mpu.x >> 8;
    p.data[1] = mpu.x;
    p.data[2] = mpu.y >> 8;
    p.data[3] = mpu.y;
    p.data[4] = mpu.z >> 8;
    p.data[5] = mpu.z;
    p.len = 6;

    rc = mcp2515_send2(&p);
    if (rc) {
        /* can error, attempt to transmit error, reinit */
        return rc;
    }

	 _delay_ms(150);
    p.type = TYPE_value_periodic;
    p.id = ID_accel;
    p.data[0] = nunchuck.accel_x >> 8;
    p.data[1] = nunchuck.accel_x;
    p.data[2] = nunchuck.accel_y >> 8;
    p.data[3] = nunchuck.accel_y;
    p.data[4] = nunchuck.accel_z >> 8;
    p.data[5] = nunchuck.accel_z;
    p.len = 6;
    rc = mcp2515_send2(&p);
    if (rc) {
        return rc;
        /* can error, attempt to transmit error, reinit */
    }

    return 0;
}

uint8_t can_irq(void)
{
    struct mcp2515_packet_t p;
    uint8_t rc;

    struct hmc5883_t hmc;
    struct mpu6050_t mpu;
    struct nunchuck_t nunchuck;

    hmc.x = -1;
    hmc.y = -1;
    hmc.z = -1;

    mpu.x = -1;
    mpu.y = -1;
    mpu.z = -1;

    nunchuck.accel_x = 255;
    nunchuck.accel_y = 255;
    nunchuck.accel_z = 255;

    switch(packet.type) {
    case TYPE_value_request:
        if(packet.tag == 0) {
            rc = hmc_read(&hmc);
            if (rc) {
                /* sensor error, reinit */
                return rc;
            }

            p.type = TYPE_value_explicit;
            p.id = MY_ID;
            p.data[0] = hmc.x >> 8;
            p.data[1] = hmc.x;
            p.data[2] = hmc.y >> 8;
            p.data[3] = hmc.y;
            p.data[4] = hmc.z >> 8;
            p.data[5] = hmc.z;
            p.tag = 0;
            p.len = 6;
            rc = mcp2515_send2(&p);
            if (rc) {
                /* can error, attempt to transmit error, reinit */
                return rc;
            }

        }
        else if(packet.tag == 1) {

            rc = mpu_read(&mpu);
            if (rc) {
                /* sensor error, reinit */
                return rc;
            }

            p.type = TYPE_value_explicit;
            p.id = MY_ID;
            p.data[0] = mpu.x >> 8;
            p.data[1] = mpu.x;
            p.data[2] = mpu.y >> 8;
            p.data[3] = mpu.y;
            p.data[4] = mpu.z >> 8;
            p.data[5] = mpu.z;
            p.tag = 1;
            p.len = 6;

            rc = mcp2515_send2(&p);
            if (rc) {
                /* can error, attempt to transmit error, reinit */
                return rc;
            }

        }
    }

    packet.unread = 0;
    return 0;
}

void main(void)
{
    NODE_INIT();

    sei();

    rc = hmc_init();
    if (rc) {
        puts_P(PSTR("hmc init"));
    }

    rc = mpu_init();
    if (rc) {
        puts_P(PSTR("mpu init"));
    }

    rc = nunchuck_init();
    if (rc) {
        puts_P(PSTR("nunch init"));
    }

    hmc_set_continuous();

    NODE_MAIN();
}
