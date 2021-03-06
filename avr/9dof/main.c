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

uint8_t debug_irq(void)
{
    return 0;
}

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
    if (irq_signal & IRQ_USER1)
        return;

    if (c == '\r') {
        irq_signal |= IRQ_USER1;
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

uint8_t send_sats_can(uint8_t type)
{
    struct mcp2515_packet_t p;

    p.type = type;
    p.id = ID_9dof;
    p.sensor = SENSOR_sats;
    p.data[0] = num_sats;
    p.len = 1;

    return mcp2515_sendpacket_wait(&p);
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

    return mcp2515_sendpacket_wait(&p);
}

uint8_t user1_irq(void)
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
        rc = mcp2515_send_wait(TYPE_sensor_error, MY_ID, SENSOR_gyro, &rc, sizeof(rc));
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
        rc = mcp2515_send_wait(TYPE_sensor_error, MY_ID, SENSOR_compass, &rc, sizeof(rc));
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
        rc = mcp2515_send_wait(TYPE_sensor_error, MY_ID, SENSOR_accel, &rc, sizeof(rc));
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

    if (now - coords_read_time > 5) {
        if (type == TYPE_value_explicit) {
            return mcp2515_send_wait(TYPE_sensor_error, MY_ID, SENSOR_coords,
                    NULL, 0);
        } else {
            return 0;
        }
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

    return mcp2515_sendpacket_wait(&p);
}

void send_all_can(uint8_t type)
{
    send_gyro_can(type);
    _delay_ms(1000);

    send_compass_can(type);
    _delay_ms(1000);

    send_accel_can(type);
    _delay_ms(1000);

    send_coords_can(type);
    _delay_ms(1000);

    send_time_can(type);
}

uint8_t uart_irq(void)
{
    return 0;
}

uint8_t periodic_irq(void)
{
    send_all_can(TYPE_value_periodic);

    return 0;
}

uint8_t can_irq(void)
{
    uint8_t uptime_buf[4];

    while (mcp2515_get_packet(&packet) == 0) {
        switch (packet.type) {
        case TYPE_value_request:
            switch (packet.sensor) {
            case SENSOR_accel:
                send_accel_can(TYPE_value_explicit);
                break;
            case SENSOR_sats:
                send_sats_can(TYPE_value_explicit);
                break;
            case SENSOR_gyro:
                send_gyro_can(TYPE_value_explicit);
                break;
            case SENSOR_compass:
                send_compass_can(TYPE_value_explicit);
                break;
            case SENSOR_coords:
                send_coords_can(TYPE_value_explicit);
                break;
            case SENSOR_time:
                send_time_can(TYPE_value_explicit);
                break;
            case SENSOR_uptime:
                uptime_buf[0] = uptime >> 24;
                uptime_buf[1] = uptime >> 16;
                uptime_buf[2] = uptime >> 8;
                uptime_buf[3] = uptime >> 0;

                mcp2515_send_sensor(TYPE_value_explicit, MY_ID, SENSOR_uptime, uptime_buf, sizeof(uptime_buf));
                break;

            case SENSOR_none:
                send_all_can(TYPE_value_explicit);
                break;

            default:
                mcp2515_send_sensor(TYPE_sensor_error, MY_ID, packet.sensor, NULL, 0);
            }
        }
    }

    return 0;
}

void sleep(void)
{
    sleep_enable();
    sleep_bod_disable();
    sei();
    sleep_cpu();
    sleep_disable();
}

int main(void)
{
    uint8_t rc;

    node_init();

    sei();

    i2c_init(I2C_FREQ(400000));

    rc = hmc_init();
    if (rc) {
//        puts_P(PSTR("hmc init"));
    }

    rc = mpu_init();
    if (rc) {
//        puts_P(PSTR("mpu init"));
    }

    rc = nunchuck_init();
    if (rc) {
//        puts_P(PSTR("nunch init"));
    }

    hmc_set_continuous();

    node_main();
}
