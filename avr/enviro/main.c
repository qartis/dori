#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "irq.h"
#include "time.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "can.h"
#include "node.h"
#include "i2c.h"
#include "bmp085.h"
#include "wind.h"
#include "humidity.h"
#include "temp.h"
#include "adc.h"

volatile uint32_t water_tips;

ISR(PCINT2_vect)
{
    if (PIND & (1 << PORTD6)) {
        water_tips++;
        _delay_us(500);
    }
}

uint8_t send_temp_can(int8_t index, uint8_t type)
{
    uint8_t buf[2];
    uint8_t rc;
    uint8_t i;
    int16_t temp;

    temp_begin();
    temp_wait();

    if (index == TEMP_ALL_CHANNELS) {
        for (i = 0; i < temp_num_sensors; i++) {
            rc = temp_read(i, &temp);
            if (rc) {
                rc = mcp2515_send_wait(TYPE_sensor_error, MY_ID,
                                       SENSOR_temp0 + i, NULL, 0);

                if (rc)
                    return rc;

                continue;
            }

            buf[0] = temp >> 8;
            buf[1] = temp & 0xFF;

            rc = mcp2515_send_wait(type, MY_ID, SENSOR_temp0 + i, buf, 2);

            if (rc)
                return rc;
        }
    } else {
        rc = temp_read(index, &temp);
        if (rc) {
            return mcp2515_send_wait(TYPE_sensor_error, MY_ID,
                    SENSOR_temp0 + index, NULL, 0);
        }

        buf[0] = temp >> 8;
        buf[1] = temp & 0xFF;

        rc = mcp2515_send_wait(type, MY_ID, SENSOR_temp0 + index, buf, 2);

        if (rc)
            return rc;
    }

    return 0;
}

uint8_t send_rain_can(uint8_t type)
{
    uint8_t buf[4];


    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        buf[0] = (water_tips >> 24) & 0xff;
        buf[1] = (water_tips >> 16) & 0xff;
        buf[2] = (water_tips >> 8) & 0xff;
        buf[3] = (water_tips >> 0) & 0xff;
        water_tips = 0;
    }

    return mcp2515_send_wait(type, MY_ID, SENSOR_rain, buf, sizeof(buf));
}

uint8_t send_wind_can(uint8_t type)
{
    uint8_t buf[2];
    uint16_t wind;

    wind = get_wind_speed();

    buf[0] = wind >> 8;
    buf[1] = (wind & 0x00FF);

    return mcp2515_send_wait(type, MY_ID, SENSOR_wind, buf, 2);
}

uint8_t send_humidity_can(uint8_t type)
{
    uint8_t buf[2];
    uint16_t humidity;

    humidity = get_humidity();

    buf[0] = humidity >> 8;
    buf[1] = (humidity & 0x00FF);

    return mcp2515_send_wait(type, MY_ID, SENSOR_humidity, buf, 2);
}

uint8_t send_pressure_can(uint8_t type)
{
    uint8_t buf[4];
    struct bmp085_sample s;
    uint32_t pressure;

    bmp085_get_cal_param();

    s = bmp085_read();

    pressure = s.pressure;

    buf[0] = (pressure >> 24) & 0xff;
    buf[1] = (pressure >> 16) & 0xff;
    buf[2] = (pressure >> 8) & 0xff;
    buf[3] = (pressure >> 0) & 0xff;

    return mcp2515_send_wait(type, MY_ID, SENSOR_pressure, buf, 4);
}

uint8_t send_all_can(uint8_t type)
{
    uint8_t rc;

    rc = send_temp_can(TEMP_ALL_CHANNELS, type);
    if (rc) {
        return rc;
    }

    rc = send_rain_can(type);
    if (rc) {
        return rc;
    }

    rc = send_wind_can(type);
    if (rc) {
        return rc;
    }

    rc = send_humidity_can(type);
    if (rc) {
        return rc;
    }

    return send_pressure_can(type);
}

uint8_t periodic_irq(void)
{
    return send_all_can(TYPE_value_periodic);
}

uint8_t can_irq(void)
{
    uint8_t rc;
    uint8_t uptime_buf[4];

    rc = 0;

    switch (packet.type) {
    case TYPE_value_request:
        switch (packet.sensor) {
        case SENSOR_temp0 ... SENSOR_temp4:
            rc = send_temp_can(packet.sensor - SENSOR_temp0,
                               TYPE_value_explicit);
            break;
        case SENSOR_rain:
            rc = send_rain_can(TYPE_value_explicit);
            break;
        case SENSOR_wind:
            rc = send_wind_can(TYPE_value_explicit);
            break;
        case SENSOR_humidity:
            rc = send_humidity_can(TYPE_value_explicit);
            break;
        case SENSOR_pressure:
            rc = send_pressure_can(TYPE_value_explicit);
            break;
        case SENSOR_uptime:
            uptime_buf[0] = uptime >> 24;
            uptime_buf[1] = uptime >> 16;
            uptime_buf[2] = uptime >> 8;
            uptime_buf[3] = uptime >> 0;

            rc = mcp2515_send_sensor(TYPE_value_explicit, MY_ID, SENSOR_uptime, uptime_buf, sizeof(uptime_buf));
            break;

        case SENSOR_none:
            rc = send_all_can(TYPE_value_explicit);
            break;

        default:
            rc = mcp2515_send_sensor(TYPE_sensor_error, MY_ID, packet.sensor, NULL, 0);
        }
    }

    return rc;
}

uint8_t uart_irq(void)
{







    int16_t temp;
    uint16_t wind;

    wind = get_wind_speed();
    printf("wind %u\n", wind);

    temp_begin();
    temp_wait();

    temp_read(0, &temp);
    printf("temp %d\n", temp);

    struct bmp085_sample s;

    bmp085_get_cal_param();

    s = bmp085_read();
    uint32_t pressure = s.pressure;

    printf("pressure %lu\n", pressure);

    uint16_t humidity = get_humidity();
    printf("humid %u\n", humidity);

    uint32_t rain;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        rain = water_tips;
        water_tips = 0;
    }

    printf("rain: %lu\n", rain);

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
    NODE_INIT();

    temp_init();
    adc_init();
    i2c_init(I2C_FREQ(400000));

    PCMSK2 |= (1 << PCINT22);
    PCICR |= (1 << PCIE2);
    PORTD |= (1 << PORTD6);

    sei();

    NODE_MAIN();
}
