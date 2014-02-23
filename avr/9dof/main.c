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
#include "debug.h"

#define TIME_SYNC_INTERVAL 60 * 60 * 2

volatile uint32_t coords_read_time;
volatile uint32_t last_time_sync;
volatile uint32_t num_sats_read_time;

volatile int32_t cur_lat;
volatile int32_t cur_lon;
volatile uint8_t num_sats;

volatile uint8_t nmea_buf[BUFLEN];
volatile uint8_t nmea_buf_len;

ISR(USART_RX_vect)
{
    uint8_t c = UDR0;

    /* if we've got an unread line */
    if (irq_signal & IRQ_USER)
        return;

    if (c == '\r') {
        irq_signal |= IRQ_USER;
        return;
    }

    if (!isprint(c)) {
        nmea_buf_len = 0;
        return;
    }

    if (c == '$')
        nmea_buf_len = 0;


    nmea_buf[nmea_buf_len] = c;

    if (nmea_buf_len < BUFLEN - 1)
        nmea_buf_len++;
}

uint8_t send_time_can(uint8_t type)
{
    struct mcp2515_packet_t p;

    p.type = type;
    p.id = ID_9dof;
    p.sensor = SENSOR_time;
    p.data[0] = (now >> 24) & 0xff;
    p.data[1] = (now >> 16) & 0xff;
    p.data[2] = (now >> 8) & 0xff;
    p.data[3] = (now >> 0) & 0xff;
    p.len = 4;

    time_set(now);

    return mcp2515_sendpacket_wait(&p);
}

uint8_t user_irq(void)
{
    struct nmea_data_t result;
    char buf[BUFLEN];
    uint8_t i;

    nmea_buf[nmea_buf_len] = '\0';

    for (i = 0; i < sizeof(buf); i++)
        buf[i] = nmea_buf[i];

    result = parse_nmea(buf);

    nmea_buf_len = 0;

    switch (result.tag) {
    case NMEA_TIMESTAMP:
        if (result.timestamp > last_time_sync + TIME_SYNC_INTERVAL) {
            time_set(result.timestamp);
            last_time_sync = result.timestamp;
            send_time_can(TYPE_value_explicit);
        }
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
        rc = mcp2515_send_wait(TYPE_sensor_error, MY_ID, &rc, sizeof(rc), SENSOR_gyro);
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

    return mcp2515_sendpacket_wait(&p);
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
        rc = mcp2515_send_wait(TYPE_sensor_error, MY_ID, &rc, sizeof(rc), SENSOR_compass);
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

    return mcp2515_sendpacket_wait(&p);
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
        rc = mcp2515_send_wait(TYPE_sensor_error, MY_ID, &rc, sizeof(rc), SENSOR_accel);
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

    return mcp2515_sendpacket_wait(&p);
}

uint8_t send_coords_can(uint8_t type)
{
    struct mcp2515_packet_t p;

    if (now - coords_read_time > 5)
        return 0;

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

    return mcp2515_sendpacket_wait(&p);
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

uint8_t uart_irq(void)
{
    return 0;
}

uint8_t debug_irq(void)
{
    char buf[64];

    fgets(buf, sizeof(buf), stdin);

    send_all_can(TYPE_value_explicit);

    return 0;
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
        case SENSOR_time:
            rc = send_time_can(TYPE_value_explicit);
            break;
        default:
            rc = send_all_can(TYPE_value_explicit);
        }
    }

    packet.unread = 0;

    return rc;
}

int main(void)
{
    NODE_INIT();

    sei();
    i2c_init(I2C_FREQ(400000));

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
