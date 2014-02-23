#include <stdio.h>
#include <string.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include "adc.h"
#include "heater.h"
#include "irq.h"
#include "time.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "can.h"
#include "node.h"
#include "i2c.h"
#include "uart.h"
#include "power.h"
#include "temp.h"

void timer_tophalf(void)
{
    irq_signal |= IRQ_USER;
}

uint8_t user_irq(void)
{
    uint8_t rc;
    int16_t temp;
    static uint8_t seconds = 0;

    seconds++;

    if (seconds < 3)
        return 0;

    seconds = 0;

    uint16_t adc_voltage = get_voltage();
    uint16_t mV;
    uint32_t V;

    V = adc_voltage * 50000; /* 5.00V */
    V >>= 10;
    V += 100000;
    mV = V % 10000;
    V = V / 10000;

    printf("%lu.%uV (%u)\n", V, mV, adc_voltage);

    if (adc_voltage < 220) {
        printf("voltage too low, disabling heaters\n");
        heater_off(0);
        heater_off(1);
        heater_off(2);
        heater_off(3);
        return 0;
    }


    uint16_t current = get_current();
    printf("current: %u\n", current);

    rc = temp_begin();
    if (rc)
        return 0;

    temp_wait();

    uint8_t i;

    for (i = 0; i < temp_num_sensors; i++) {
        rc = temp_read(i, &temp);
        if (rc)
            return 0;

        if (temp < 170)
            heater_on(i);
        else
            heater_off(i);
    }

    return 0;
}

uint8_t send_temp_can(int8_t index, uint8_t type)
{
    uint8_t buf[2];
    uint8_t rc;

    temp_begin();
    temp_wait();

    int16_t temp;

    // -1 means send all temperature sensor readings
    if (index == -1) {
        uint8_t i = 0;
        for (i = 0; i < temp_num_sensors; i++) {
            rc = temp_read(i, &temp);
            if (rc) {
                rc = mcp2515_send_wait(
                        TYPE_sensor_error, MY_ID,
                        NULL, 0, SENSOR_temp5 + i);
                /* what should we do here? sensor
                measurement screwed up, but we
                reported it successfully. */
                return rc;
            }

            buf[0] = temp >> 8;
            buf[1] = temp & 0xFF;

            printf("temp: %d\n", temp);

            rc = mcp2515_send_wait(type, MY_ID,
                    buf, 2, SENSOR_temp5 + i);
            if (rc)
                return rc;
        }
    } else {
        rc = temp_read(index, &temp);
        if (rc) {
            rc = mcp2515_send_wait(TYPE_sensor_error,
                    MY_ID, NULL, 0, 
                    SENSOR_temp5 + index);
            /* what should we do here? sensor
               measurement screwed up, but we
               reported it successfully. */
            return rc;
        }

        buf[0] = temp >> 8;
        buf[1] = temp & 0xFF;

        rc = mcp2515_send_wait(type, MY_ID, buf,
                2, SENSOR_temp5 + index);
        if (rc) {
            return rc;
        }
    }

    return 0;
}

uint8_t send_voltage_can(uint8_t type)
{
    char buf[8];
    uint16_t adc_voltage = get_voltage();
    uint16_t mV;
    uint32_t V;

    V = adc_voltage * 50000; /* 5.00V */
    V >>= 10;
    V += 100000;
    mV = V % 10000;
    V = V / 10000;

    buf[0] = adc_voltage >> 8;
    buf[1] = (adc_voltage & 0x00FF);

    snprintf(buf + 2, sizeof(buf) - 2, 
            "%lu.%u", V, mV);

    printf("voltage: %lu.%uV (%u)\n", V, mV, 
            adc_voltage);

    return mcp2515_send_wait(type, MY_ID, buf,
            sizeof(buf), SENSOR_voltage);
}


uint8_t send_current_can(uint8_t type)
{
    uint8_t buf[2];
    uint16_t current = get_current();
    buf[0] = current >> 8;
    buf[1] = (current & 0x00FF);

    return mcp2515_send_wait(type, MY_ID, buf,
            2, SENSOR_current);
}

uint8_t send_all_can(uint8_t type)
{
    uint8_t rc;
    printf("send all can\n");
    // -1 means send all temperature sensor readings
    rc = send_temp_can(-1, type);
    if (rc) {
        return rc;
    }

    _delay_ms(150);

    rc = send_voltage_can(type);
    if (rc) {
        return rc;
    }

    _delay_ms(150);
    rc = send_current_can(type);
    return rc;
}

uint8_t periodic_irq(void)
{
    uint8_t rc;

    printf("hey\n");

    return 0;

    // -1 means send all temperature sensor readings
    rc = send_temp_can(-1, TYPE_value_periodic);
    if (rc) {
        printf("send_temp_can()\n");
        return rc;
    }

    _delay_ms(550);

    rc = send_voltage_can(TYPE_value_periodic);
    if (rc) {
        printf("send_voltage_can() = %u\n", rc);
        return rc;
    }

#if 0
    _delay_ms(150);
    rc = send_current_can(TYPE_value_periodic);
    return rc;
#endif

    return 0;
}

uint8_t can_irq(void)
{
    printf("can irq\n");

    switch(packet.type) {
    case TYPE_value_request:
        switch(packet.sensor) {
        case SENSOR_temp5 ... SENSOR_temp8:
            send_temp_can(packet.sensor - SENSOR_temp5,
                          TYPE_value_explicit);
            break;
        case SENSOR_voltage:
            send_voltage_can(TYPE_value_explicit);
            break;
        case SENSOR_current:
            send_current_can(TYPE_value_explicit);
            break;
        default:
            /* send everything */
            send_all_can(TYPE_value_explicit);
        }
        break;
    default:
        break;
    }

    packet.unread = 0;

    return 0;
}

uint8_t uart_irq(void)
{
    char buf[UART_BUF_SIZE];

    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';

    if (buf[0] == '\0') {
        send_temp_can(-1, 0);
        uint16_t current = get_current();
        printf("I: %u\n", current);


        uint16_t adc_voltage = get_voltage();
        uint16_t mV;
        uint32_t V;

        V = adc_voltage * 53500; /* 5.00V */
        V >>= 10;
        V += 100000;
        mV = V % 10000;
        V = V / 10000;

        buf[0] = adc_voltage >> 8;
        buf[1] = (adc_voltage & 0x00FF);

        printf("voltage: %lu.%uV (%u)\n", V, mV, 
                adc_voltage);



    } else {
        heater_on(0);
        heater_on(1);
        heater_on(2);
        heater_on(3);

        _delay_ms(5000);

        _delay_ms(250);
        heater_off(0);
        _delay_ms(250);
        heater_off(1);
        _delay_ms(250);
        heater_off(2);
        _delay_ms(250);
        heater_off(3);

        send_voltage_can(TYPE_value_explicit);
    }

    return 0;
}

int main(void)
{
    NODE_INIT();

    heater_init();
    temp_init();
    adc_init();
    i2c_init(I2C_FREQ(400000));

    sei();

    NODE_MAIN();
}
