#include <stdio.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <util/delay.h>

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

volatile uint8_t water_tips;

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

    if (num_sensors == 0) {
        temp_init();
    }

    temp_begin();
    temp_wait();

    int16_t temp;

    // -1 means send all temperature sensor readings
    if (index == -1) {
        uint8_t i = 0;
        for (i = 0; i < num_sensors; i++) {
            temp_read(i, &temp);
            buf[0] = temp >> 8;
            buf[1] = temp & 0xFF;

            rc = mcp2515_send_sensor(type, MY_ID, buf, 2, SENSOR_temp0 + i);
            if (rc) {
                return rc;
            }

            _delay_ms(150);
        }
    } else {
        temp_read(index, &temp);
        buf[0] = temp >> 8;
        buf[1] = temp & 0xFF;

        rc = mcp2515_send_sensor(type, MY_ID, buf, 2, SENSOR_temp0 + index);
        if (rc) {
            return rc;
        }
    }

    return 0;
}

uint8_t send_rain_can(uint8_t type)
{
    uint8_t buf[1];
    uint8_t rc;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        buf[0] = water_tips;
        water_tips = 0;
    }

    rc = mcp2515_send_sensor(type, MY_ID, buf, 1, SENSOR_rain);

    water_tips = 0;

    return rc;
}

uint8_t send_wind_can(uint8_t type)
{
    uint8_t buf[2];
    uint16_t wind;

    wind = get_wind_speed();
    buf[0] = wind >> 8;
    buf[1] = (wind & 0x00FF);

    return mcp2515_send_sensor(type, MY_ID, buf, 2, SENSOR_wind);
}

uint8_t send_humidity_can(uint8_t type)
{
    uint8_t buf[2];
    uint16_t humidity = get_humidity();
    buf[0] = humidity >> 8;
    buf[1] = (humidity & 0x00FF);

    return mcp2515_send_sensor(type, MY_ID, buf, 2, SENSOR_humidity);
}

uint8_t send_pressure_can(uint8_t type)
{
    uint8_t buf[4];
    struct bmp085_sample s;

    bmp085_get_cal_param();

    s = bmp085_read();
    uint32_t pressure = s.pressure;
    buf[0] = pressure >> 24;
    buf[1] = pressure >> 16;
    buf[2] = pressure >> 8;
    buf[3] = pressure & 0xFF;

    return mcp2515_send_sensor(type, MY_ID, buf, 4, SENSOR_pressure);
}

uint8_t send_all_can(uint8_t type)
{
    uint8_t rc;
    // -1 means send all temperature sensor readings
    rc = send_temp_can(-1, type);
    if (rc) {
        return rc;
    }

    _delay_ms(150);
    rc = send_rain_can(type);
    if (rc) {
        return rc;
    }

    _delay_ms(150);
    rc = send_wind_can(type);
    if (rc) {
        return rc;
    }

    _delay_ms(150);
    rc = send_humidity_can(type);
    if (rc) {
        return rc;
    }

    _delay_ms(150);
    rc = send_pressure_can(type);
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
        default:
            rc = send_all_can(TYPE_value_explicit); // send everything
        }
    }

    return rc;
}

uint8_t uart_irq(void)
{
    return 0;
}

void main(void)
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
