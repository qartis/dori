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
#include "uart.h"
#include "voltage.h"
#include "current.h"
#include "temp.h"

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
                                SENSOR_temp5 + i);

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

void send_voltage_can(void)
{
    uint8_t buf[2];
    uint16_t voltage = get_voltage();
    buf[0] = voltage >> 8;
    buf[1] = (voltage & 0x00FF);

    mcp2515_send_sensor(TYPE_value_periodic,
                        MY_ID,
                        buf,
                        2,
                        SENSOR_voltage);
}


void send_current_can(void)
{
    uint8_t buf[2];
    uint16_t current = get_current();
    buf[0] = current >> 8;
    buf[1] = (current & 0x00FF);

    mcp2515_send_sensor(TYPE_value_periodic,
                        MY_ID,
                        buf,
                        2,
                        SENSOR_current);
}

uint8_t periodic_irq(void)
{
    // -1 means send all temperature sensor readings
    send_temp_can(-1);
    _delay_ms(150);

    send_voltage_can();
    _delay_ms(150);

    send_current_can();
    _delay_ms(150);

    return 0;
}

uint8_t can_irq(void)
{
    switch(packet.type) {
    case TYPE_value_request:
        switch(packet.sensor) {
        case SENSOR_temp5 ... SENSOR_temp8:
            send_temp_can(packet.sensor - SENSOR_temp0);
            break;
        case SENSOR_voltage:
            send_voltage_can();
            break;
        case SENSOR_current:
            send_current_can();
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
