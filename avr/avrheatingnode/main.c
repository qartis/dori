#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "uart.h"
#include "mcp2515.h"
#include "time.h"
#include "output.h"
#include "ds18b20.h"
#include "onewire.h"
#include "spi.h"

#define STR(X) #X

volatile uint8_t int_signal;
volatile struct mcp2515_packet_t p;

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
        puts_P(PSTR("CRC Error"));
        return 1;
    }

    printf_P("chan %d: %d\n", channel, *temp);

    return 0;
}

void periodic_callback(void)
{
    /* this also gets called when we move the arm */
    uint8_t rc;
    int16_t temp;

    uint8_t channel = 0;
    
    rc = read_temp(channel, &temp);
    if (rc != 0) {
        rc = mcp2515_send(TYPE_sensor_error, ID_heater, 1, (void *)&rc);
        /* assume that sent */
        return;
    }

    uint8_t buf[3];
    buf[0] = channel;
    buf[1] = (temp >> 8) & 0xff;
    buf[2] = temp & 0xff;

    rc = mcp2515_send(TYPE_value_periodic, ID_heater, 3, buf);
    if (rc != 0) {
        //how to handle error here?
    }
}

void mcp2515_irq_callback(void)
{
    int_signal = 1;
    p = packet;
}

void bottom_half(void)
{
    uint8_t device_num;

    switch (p.type) {
    case TYPE_set_output:
        if (p.len != 1) {
            puts_P(PSTR("err: TYPE_output_enable data len"));
            break;
        }
        device_num = p.data[0];
       // output_on(device_num);
      //  send_device_status(device_num);
        break;

    case TYPE_get_output:
        if (p.len != 1) {
            puts_P(PSTR("err: TYPE_get_output data len"));
            break;
        }
        device_num = p.data[0];
     //   send_device_status(device_num);
        break;

    case TYPE_get_value:
        if (p.len != 1) {
            puts_P(PSTR("err: TYPE_get_value data len"));
            break;
        }
        device_num = p.data[0];
//        send_device_status(device_num);
        break;

    default:
        break;
    }
}

void main(void)
{
    uart_init(BAUD(38400));
    time_init();
    spi_init();

    _delay_ms(200);
    puts_P(PSTR("\n\n" STR(MY_ID) " node start"));

    while (mcp2515_init()) {
        puts_P(PSTR("mcp: init"));
        _delay_ms(500);
    }

    sei();

    for (;;) {
        printf_P(PSTR(">| "));

        while (int_signal == 0) {};

        int_signal = 0;
        bottom_half();
    }
}
