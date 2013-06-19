#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

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

void uart_irq(void)
{
    char buf[UART_BUF_SIZE];

    fgets(buf, sizeof(buf), stdin);

    parse_nmea(buf);
}

void periodic_irq(void)
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
    }

    rc = mpu_read(&mpu);
    if (rc) {
        /* sensor error, reinit */
    }

    rc = nunchuck_read(&nunchuck);

	 if (rc) {
        /* sensor error, reinit */
	 }
	 p.type = TYPE_value_periodic;
    p.id = ID_compass;
    p.data[0] = (uint8_t)(mpu.x >> 8);
    p.data[1] = (uint8_t)mpu.x;
    p.data[2] = (uint8_t)(mpu.y >> 8);
    p.data[3] = (uint8_t)mpu.y;
    p.data[4] = (uint8_t)(mpu.z >> 8);
    p.data[5] = (uint8_t)mpu.z;
    p.len = 6;
    rc = mcp2515_send2(&p);
    if (rc) {
        /* can error, attempt to transmit error, reinit */
    }

	 _delay_ms(150);
	 p.type = TYPE_value_periodic;
    p.id = ID_gyro;
    p.data[0] = (uint8_t)(hmc.x >> 8);
    p.data[1] = (uint8_t)hmc.x;
    p.data[2] = (uint8_t)(hmc.y >> 8);
    p.data[3] = (uint8_t)hmc.y;
    p.data[4] = (uint8_t)(hmc.z >> 8);
    p.data[5] = (uint8_t)hmc.z;
    p.len = 6;
    rc = mcp2515_send2(&p);
    if (rc) {
        /* can error, attempt to transmit error, reinit */
    }
	 _delay_ms(150);
    p.type = TYPE_value_periodic;
    p.id = ID_accel;
    p.data[0] = (uint8_t)(nunchuck.accel_x >> 8);
    p.data[1] = (uint8_t)nunchuck.accel_x;
    p.data[2] = (uint8_t)(nunchuck.accel_y >> 8);
    p.data[3] = (uint8_t)nunchuck.accel_y;
    p.data[4] = (uint8_t)(nunchuck.accel_z >> 8);
    p.data[5] = (uint8_t)nunchuck.accel_z;
    p.len = 6;
    rc = mcp2515_send2(&p);
    if (rc) {
        /* can error, attempt to transmit error, reinit */
    }
}

void can_irq(void)
{
		  packet.unread = 0;
}

void main(void)
{
    uint8_t rc;

    NODE_INIT();

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

    for (;;) {
        printf_P(PSTR(XSTR(MY_ID) "> "));

        while (irq_signal == 0) {};

        if (irq_signal & IRQ_CAN) {
            can_irq();
            irq_signal &= ~IRQ_CAN;
        }

        if (irq_signal & IRQ_TIMER) {
            periodic_irq();
            irq_signal &= ~IRQ_TIMER;
        }

        if (irq_signal & IRQ_UART) {
            uart_irq();
            irq_signal &= ~IRQ_UART;
        }
    }
}
