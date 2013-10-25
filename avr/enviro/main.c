#include <stdio.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

#define UART_BAUD 38400

#include "irq.h"
#include "time.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "can.h"
#include "node.h"

#include "i2c.h"
#include "bmp085.h"
#include "uart.h"
#include "wind.h"
#include "humidity.h"
#include "temp.h"

volatile uint8_t water_tips;

ISR(PCINT2_vect)
{
    if (PIND & (1 << PORTD6)) {
        water_tips++;
        _delay_us(500);
    }
}

void send_temp_can(int8_t index)
{
    uint8_t buf[2];

    if (num_sensors == 0) {
        temp_init();
    }

    temp_begin();
    temp_wait();

    int16_t temp;

    // -1 means send all temperature sensor readings
    if(index == -1)
    {
        uint8_t i = 0;
        for (i = 0; i < num_sensors; i++) {
            temp_read(i, &temp);
            buf[0] = temp >> 8;
            buf[1] = temp & 0xFF;

            mcp2515_send_sensor(TYPE_value_periodic,
                                MY_ID,
                                buf,
                                2,
                                SENSOR_temp0 + i);

            _delay_ms(150);
        }
    }
    else {
        temp_read(index, &temp);
        buf[0] = temp >> 8;
        buf[1] = temp & 0xFF;

        mcp2515_send_sensor(TYPE_value_periodic,
                            MY_ID,
                            buf,
                            2,
                            SENSOR_temp0 + index);
    }
}

void send_rain_can(void)
{
    uint8_t buf[1];

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        buf[0] = water_tips;
        water_tips = 0;
    }

    mcp2515_send_sensor(TYPE_value_periodic,
                        MY_ID,
                        buf,
                        1,
                        SENSOR_rain);

    water_tips = 0;
}

void send_wind_can(void)
{
    uint8_t buf[2];
    uint16_t wind = get_wind_speed();
    buf[0] = wind >> 8;
    buf[1] = (wind & 0x00FF);

    mcp2515_send_sensor(TYPE_value_periodic,
                        MY_ID,
                        buf,
                        2,
                        SENSOR_wind);
}


void send_humidity_can(void)
{
    uint8_t buf[2];
    uint16_t humidity = get_humidity();
    buf[0] = humidity >> 8;
    buf[1] = (humidity & 0x00FF);

    mcp2515_send_sensor(TYPE_value_periodic,
                        MY_ID,
                        buf,
                        2,
                        SENSOR_humidity);
}


void send_pressure_can(void)
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

    mcp2515_send_sensor(TYPE_value_periodic,
                        MY_ID,
                        buf,
                        4,
                        SENSOR_pressure);

}

uint8_t periodic_irq(void)
{
    // -1 means send all temperature sensor readings
    send_temp_can(-1);
    _delay_ms(150);

    send_rain_can();
    _delay_ms(150);

    send_wind_can();
    _delay_ms(150);

    send_humidity_can();
    _delay_ms(150);

    send_pressure_can();
    return 0;
}

uint8_t can_irq(void)
{
    switch(packet.type) {
    case TYPE_value_request:
        switch(packet.sensor) {
        case SENSOR_temp0 ... SENSOR_temp4:
            send_temp_can(packet.sensor - SENSOR_temp0);
            break;
        case SENSOR_rain:
            send_rain_can();
            break;
        case SENSOR_wind:
            send_wind_can();
            break;
        case SENSOR_humidity:
            send_humidity_can();
            break;
        case SENSOR_pressure:
            send_pressure_can();
            break;
        default:
            periodic_irq(); // send everything
        }
    }

    return 0;
}

uint8_t uart_irq(void)
{
    return 0;
}

void main(void)
{
    temp_init();
    adc_init();
    i2c_init(I2C_FREQ(400000));

    PCMSK2 |= (1 << PCINT22);
    PCICR |= (1 << PCIE2);
    PORTD |= (1 << PORTD6);

    NODE_INIT();
    sei();

    NODE_MAIN();
}
