#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <ctype.h>

#include "irq.h"
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
#include "can.h"

#define streq_P(a,b) (strcmp_P(a,b) == 0)

volatile uint8_t gps_read_flag;
volatile int32_t cur_lat;
volatile int32_t cur_lon;

extern volatile uint8_t uart_ring[UART_BUF_SIZE];
extern volatile uint8_t ring_in;
extern volatile uint8_t ring_out;

#define UDR UDR0

ISR(USART_RX_vect)
{
    uint8_t data = UDR;
    uart_ring[ring_in] = data;
    ring_in = (ring_in + 1) % UART_BUF_SIZE;

    if(gps_read_flag == 0) {
        irq_signal |= IRQ_USER;
    }
}

#define BUFLEN 80+2+1 /* 80 lines of text, plus possible \r\n, plus NULL */

uint8_t user_irq(void)
{
    static char buffer[BUFLEN];
    static uint8_t buflen = 0;

    while(uart_haschar()) {
        if (buflen > sizeof(buffer)){
            buflen = 0;
            continue;
        }
        char c = getchar();
        if (!isprint(c) && c != '\n'){
            continue;
        }
        if (c == '\n'){
            if (buflen > 0){
                buffer[buflen] = '\0';
                parse_nmea(buffer, &cur_lat, &cur_lon);
                gps_read_flag = 1;
            }
            buflen = 0;
            continue;
        }
        buffer[buflen++] = c;
    }

    return 0;
}


uint8_t uart_irq(void)
{
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
    p.id = ID_9dof;
    p.sensor = SENSOR_gyro;
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
    p.id = ID_9dof;
    p.sensor = SENSOR_compass;
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
    p.id = ID_9dof;
    p.sensor = SENSOR_accel;
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

    gps_read_flag = 0;
    _delay_ms(2000);
    if(gps_read_flag)
    {
        p.type = TYPE_value_periodic;
        p.id = ID_9dof;
        p.sensor = SENSOR_gps;
        p.data[0] = cur_lat >> 24;
        p.data[1] = cur_lat >> 16;
        p.data[2] = cur_lat >> 8;
        p.data[3] = cur_lat & 0xFF;
        p.data[4] = cur_lon >> 24;
        p.data[5] = cur_lon >> 16;
        p.data[6] = cur_lon >> 8;
        p.data[7] = cur_lon & 0xFF;
        p.len = 8;
        rc = mcp2515_send2(&p);

        gps_read_flag = 0;

        if (rc) {
            return rc;
            /* can error, attempt to transmit error, reinit */
        }
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
        if(packet.sensor == SENSOR_gyro) {
            rc = hmc_read(&hmc);
            if (rc) {
                /* sensor error, reinit */
                return rc;
            }

            p.type = TYPE_value_explicit;
            p.id = MY_ID;
            p.sensor = SENSOR_gyro;
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

        }
        else if(packet.sensor == SENSOR_compass) {

            rc = mpu_read(&mpu);
            if (rc) {
                /* sensor error, reinit */
                return rc;
            }

            p.type = TYPE_value_explicit;
            p.id = MY_ID;
            p.sensor = SENSOR_compass;
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
        }
        else if(packet.sensor == SENSOR_gps) {
            gps_read_flag = 0;
            _delay_ms(1000);
            if(gps_read_flag)
            {
                p.type = TYPE_value_periodic;
                p.id = ID_9dof;
                p.sensor = SENSOR_gps;
                p.data[0] = cur_lat >> 24;
                p.data[1] = cur_lat >> 16;
                p.data[2] = cur_lat >> 8;
                p.data[3] = cur_lat & 0xFF;
                p.data[4] = cur_lon >> 24;
                p.data[5] = cur_lon >> 16;
                p.data[6] = cur_lon >> 8;
                p.data[7] = cur_lon & 0xFF;
                p.len = 8;
                rc = mcp2515_send2(&p);

                gps_read_flag = 0;

                if (rc) {
                    return rc;
                    /* can error, attempt to transmit error, reinit */
                }
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

    NODE_MAIN();
}
