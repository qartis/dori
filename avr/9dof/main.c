#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
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

volatile uint32_t coords_read_time;
volatile uint32_t num_sats_read_time;

volatile int32_t cur_lat;
volatile int32_t cur_lon;
volatile uint8_t num_sats;

extern volatile uint8_t uart_ring[UART_BUF_SIZE];
extern volatile uint8_t ring_in;
extern volatile uint8_t ring_out;

#define UDR UDR0

ISR(USART_RX_vect)
{
    uint8_t data = UDR;
    uart_ring[ring_in] = data;
    ring_in = (ring_in + 1) % UART_BUF_SIZE;
    irq_signal |= IRQ_USER;
}

uint8_t send_time_can(void)
{
    struct mcp2515_packet_t p;
    uint8_t rc;

    p.type = TYPE_value_periodic;
    p.id = ID_9dof;
    p.sensor = SENSOR_time;
    p.data[0] = now >> 24;
    p.data[1] = now >> 16;
    p.data[2] = now >> 8;
    p.data[3] = now & 0xFF;
    p.len = 4;

    rc = mcp2515_send2(&p);
    return rc;
}

/* 80 lines of text, plus possible \r\n, plus NULL */
#define BUFLEN 80+2+1

uint8_t user_irq(void)
{
    static char buffer[BUFLEN];
    static uint8_t buflen = 0;

    while (uart_haschar()) {
        if (buflen > sizeof(buffer)) {
            buflen = 0;
            continue;
        }

        char c = getchar();

        if (c == '$') {
            buflen = 0;
        }

        if (!isprint(c) && c != '\n') {
            continue;
        }

        if (c == '\n') {
            if (buflen > 0) {
                buffer[buflen] = '\0';

                struct nmea_data_t result = parse_nmea(buffer);

                switch (result.tag) {
                case NMEA_TIMESTAMP:
                    time_set(result.timestamp);
                    send_time_can();
                    break;
                case NMEA_COORDS:
                    coords_read_time = now;
                    cur_lat = result.cur_lat;
                    cur_lon = result.cur_lon;
                    break;
                case NMEA_NUM_SATS:
                    num_sats_read_time = now;
                    num_sats = result.num_sats;
                    break;
                default:
                    break;
                }

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

uint8_t send_gyro_can(uint8_t type)
{
    struct mcp2515_packet_t p;
    struct hmc5883_t hmc;
    uint8_t rc;

    hmc.x = -1;
    hmc.y = -1;
    hmc.z = -1;

    rc = hmc_read(&hmc);
    if (rc) {
        /* sensor error, reinit */
        return rc;
    }

    p.type = type;
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
    return rc;
}

uint8_t send_compass_can(uint8_t type)
{
    struct mcp2515_packet_t p;
    struct mpu6050_t mpu;
    uint8_t rc;

    mpu.x = -1;
    mpu.y = -1;
    mpu.z = -1;

    rc = mpu_read(&mpu);
    if (rc) {
        /* sensor error, reinit */
        return rc;
    }

    p.type = type;
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
    return rc;
}

uint8_t send_accel_can(uint8_t type)
{
    struct mcp2515_packet_t p;
    struct nunchuck_t nunchuck;
    uint8_t rc;

    nunchuck.accel_x = 255;
    nunchuck.accel_y = 255;
    nunchuck.accel_z = 255;

    rc = nunchuck_read(&nunchuck);
    if (rc) {
        /* sensor error, reinit */
        return rc;
    }

    p.type = type;
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
    return rc;
}

uint8_t send_coords_can(uint8_t type)
{
    struct mcp2515_packet_t p;
    uint8_t rc;

    if (now - coords_read_time > 5) {
        return 0;
    }

    p.type = type;
    p.id = ID_9dof;
    p.sensor = SENSOR_coords;
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
    return rc;
}

uint8_t send_all_can(uint8_t type)
{
    uint8_t rc;

    rc = send_gyro_can(type);
    if (rc) {
        return rc;
    }

    _delay_ms(150);
    rc = send_compass_can(type);
    if (rc) {
        return rc;
    }

    _delay_ms(150);
    rc = send_accel_can(type);
    if (rc) {
        return rc;
    }

    _delay_ms(150);
    rc = send_coords_can(type);
    return rc;
}

uint8_t periodic_irq(void)
{
    return send_all_can(TYPE_value_periodic);
}

uint8_t can_irq(void)
{
    uint8_t rc = 0;

    switch (packet.type) {
    case TYPE_value_request:
        switch (packet.sensor) {
        case SENSOR_gyro:
            rc = send_gyro_can(TYPE_value_explicit);
            break;
        case SENSOR_compass:
            rc = send_compass_can(TYPE_value_explicit);
            break;
        case SENSOR_coords:
            rc = send_coords_can(TYPE_value_explicit);
            break;
        default:
            rc = send_all_can(TYPE_value_explicit);
        }
    }

    packet.unread = 0;
    return rc;
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
