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
#include "uart.h"
#include "power.h"
#include "temp.h"

uint8_t seconds_irq(void)
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
    uint8_t i;

    V = adc_voltage * 50000;    /* 5.00V */
    V >>= 10; /*  / 1024 */
    V += 100000; /* add 10V */
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
    int16_t temp;
    uint8_t i;

    temp_begin();
    temp_wait();

    if (index == TEMP_ALL_CHANNELS) {
        for (i = 0; i < temp_num_sensors; i++) {
            rc = temp_read(i, &temp);
            if (rc) {
                rc = mcp2515_send_wait(TYPE_sensor_error, MY_ID,
                        SENSOR_temp5 + i, NULL, 0);

                if (rc) {
                    return rc;
                }

                continue;
            }

            buf[0] = temp >> 8;
            buf[1] = temp & 0xFF;

            printf("temp: %d\n", temp);

            rc = mcp2515_send_wait(type, MY_ID, SENSOR_temp5 + i, buf, 2);

            if (rc) {
                return rc;
            }
        }
    } else {
        rc = temp_read(index, &temp);
        if (rc) {
            rc = mcp2515_send_wait(TYPE_sensor_error, MY_ID,
                    SENSOR_temp5 + index, NULL, 0);

            if (rc) {
                return rc;
            }
        }

        buf[0] = temp >> 8;
        buf[1] = temp & 0xFF;

        rc = mcp2515_send_wait(type, MY_ID, SENSOR_temp5 + index, buf, 2);

        if (rc) {
            return rc;
        }
    }

    return 0;
}

uint8_t send_voltage_can(uint8_t type)
{
    char buf[8];
    uint16_t adc_voltage;
    uint16_t mV;
    uint32_t V;

    adc_voltage = get_voltage();

    V = adc_voltage * 50000;    /* 5.0V */
    V >>= 10;
    V += 100000;                /* 10.0V */
    mV = V % 10000;
    V = V / 10000;

    buf[0] = adc_voltage >> 8;
    buf[1] = (adc_voltage & 0x00FF);

    snprintf(buf + 2, sizeof(buf) - 2, "%lu.%u", V, mV);

    printf("voltage: %lu.%uV (%u)\n", V, mV, adc_voltage);

    return mcp2515_send_wait(type, MY_ID, SENSOR_voltage, buf, sizeof(buf));
}

uint8_t send_current_can(uint8_t type)
{
    uint8_t buf[2];
    uint16_t current;

    _delay_ms(500);

    current = get_current();

    buf[0] = current >> 8;
    buf[1] = (current & 0x00FF);

    return mcp2515_send_wait(type, MY_ID, SENSOR_current, buf, 2);
}

uint8_t send_all_can(uint8_t type)
{
    uint8_t rc;

    rc = send_temp_can(TEMP_ALL_CHANNELS, type);
    if (rc) {
        return rc;
    }

    _delay_ms(500);

    rc = send_voltage_can(type);
    if (rc) {
        return rc;
    }

    _delay_ms(500);

    return send_current_can(type);
}

uint8_t periodic_irq(void)
{
    return send_all_can(TYPE_value_periodic);
}

uint8_t can_irq(void)
{
    uint8_t can_buf[4];

    while (mcp2515_get_packet(&packet) == 0) {
        switch (packet.type) {
        case TYPE_value_request:
            switch (packet.sensor) {
            case SENSOR_temp5 ... SENSOR_temp8:
                send_temp_can(packet.sensor - SENSOR_temp5, TYPE_value_explicit);
                break;
            case SENSOR_voltage:
                send_voltage_can(TYPE_value_explicit);
                break;
            case SENSOR_current:
                send_current_can(TYPE_value_explicit);
                break;
            case SENSOR_uptime:
                can_buf[0] = uptime >> 24;
                can_buf[1] = uptime >> 16;
                can_buf[2] = uptime >> 8;
                can_buf[3] = uptime >> 0;

                mcp2515_send_sensor(TYPE_value_explicit, MY_ID, SENSOR_uptime, can_buf, 4);
                break;
            case SENSOR_boot:
                can_buf[0] = boot_mcusr;
                mcp2515_send_sensor(TYPE_value_explicit, MY_ID, SENSOR_boot, can_buf, 1);
                break;
            case SENSOR_interval:
                can_buf[0] = periodic_interval >> 8;
                can_buf[1] = periodic_interval;
                mcp2515_send_sensor(TYPE_value_explicit, MY_ID, SENSOR_interval, can_buf, 1);
                break;
            case SENSOR_none:
                send_all_can(TYPE_value_explicit);
                break;
            default:
                mcp2515_send_sensor(TYPE_sensor_error, MY_ID, packet.sensor, NULL, 0);
            }
            break;
        default:
            break;
        }
    }

    return 0;
}

uint8_t debug_irq(void)
{
    return 0;
}

uint8_t uart_irq(void)
{
    uint8_t rc;
    char buf[UART_BUF_SIZE];

    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';

    rc = node_debug_common(buf);
    if (rc == 0) {
        return 0;
    }

    if (buf[0] == '\0') {
        _delay_ms(1000);
        uint16_t current = get_current();
        printf("I: %umA\n", current);

        uint16_t adc_voltage = get_voltage();
        uint16_t mV;
        uint32_t V;

        V = adc_voltage * 53500;    /* 5.00V */
        V >>= 10;
        V += 100000;
        mV = V % 10000;
        V = V / 10000;

        buf[0] = adc_voltage >> 8;
        buf[1] = (adc_voltage & 0x00FF);

        printf("voltage: %lu.%uV (%u)\n", V, mV, adc_voltage);
    }

    return 0;
}

void sleep(void)
{
}

int main(void)
{
    sei();
    node_init();

    heater_init();
    temp_init();
    adc_init();

    sei();

    node_main();
}
