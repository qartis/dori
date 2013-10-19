#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "node.h"
#include "irq.h"
#include "../can.h"
#include "uart.h"
#include "mcp2515.h"
#include "time.h"
#include "ds18b20.h"
#include "onewire.h"
#include "spi.h"

uint8_t num_sensors;

uint8_t read_temp(uint8_t channel, int16_t *temp)
{
    if (num_sensors == 0) {
        num_sensors = search_sensors();
        printf_P(PSTR("%d sensors found\n"), num_sensors);

        if (num_sensors == 0) {
            return 1;
        }
    }

    if (ds18b20_start_meas(DS18B20_POWER_PARASITE, NULL)) {
        puts_P(PSTR("short circuit?"));
        num_sensors = 0;
        return 1;
    }

    _delay_ms(DS18B20_TCONV_12BIT);

    if (ds18b20_read_decicelsius(&sensor_ids[channel][0], temp)) {
        puts_P(PSTR("CRC Error? lost sensor?"));
        num_sensors = 0;
        return 1;
    }

    printf_P(PSTR("chan %d: %d\n"), channel, *temp);

    return 0;
}

uint8_t uart_irq(void)
{
    char buf[UART_BUF_SIZE];

    fgets(buf, sizeof(buf), stdin);

    printf("got: '%s'\n", buf);
    int16_t temp;

    uint8_t channel = 0;
    
    uint8_t rc = read_temp(channel, &temp);
    printf("temp %d\n", temp);

    return 0;
}

uint8_t periodic_irq(void)
{
    uint8_t rc;
    int16_t temp;

    uint8_t channel = 0;
    
    rc = read_temp(channel, &temp);
    if (rc != 0) {
        //rc = mcp2515_send(TYPE_sensor_error, ID_heater, 1, (void *)&rc);
        /* assume that sent */
        //return rc;
    }

    uint8_t buf[3];
    buf[0] = channel;
    buf[1] = (temp >> 8) & 0xff;
    buf[2] = temp & 0xff;

    //rc = mcp2515_send(TYPE_value_periodic, ID_heater, 3, buf);
    //if (rc != 0) {
    //    return rc;
   // }

    return 0;
}

uint8_t can_irq(void)
{
    packet.unread = 0;

    return 0;

#if 0
    switch (packet.type) {
    case TYPE_set_output:
        if (packet.len != 1) {
            puts_P(PSTR("err: TYPE_output_enable data len"));
            break;
        }
        //device_num = packet.data[0];
        //output_on(device_num);
        //send_device_status(device_num);
        break;

    case TYPE_get_output:
        if (packet.len != 1) {
            puts_P(PSTR("err: TYPE_get_output data len"));
            break;
        }
        //device_num = packet.data[0];
        //send_device_status(device_num);
        break;

    case TYPE_get_value:
        if (packet.len != 1) {
            puts_P(PSTR("err: TYPE_get_value data len"));
            break;
        }
        //device_num = packet.data[0];
        //send_device_status(device_num);
        break;

    default:
        break;
    }
#endif
}

void main(void)
{
    NODE_INIT();

    sei();

    NODE_MAIN();
}
