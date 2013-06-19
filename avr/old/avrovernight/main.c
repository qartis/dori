#include <stdlib.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/interrupt.h>

#include "uart.h"
#include "modem.h"
#include "debug.h"
//#include "onewire.h"
//#include "ds18x20.h"
//#include "i2c.h"
//#include "nunchuck.h"

int main(void)
{
    /*
    uint8_t ntemp = 0;
    struct nunchuck n;
    int16_t temp;
    */
    uint8_t i;
    uint8_t rc;

    struct {
        uint8_t modem : 1;
        uint8_t nunchuck : 1;
        uint8_t temp : 1;
    } error;

    //i2c_init(I2C_FREQ(400000));
	uart_init(BAUD(9600));

    debug_init();
    debug("init done\n");

    error.modem = 1;
    error.nunchuck = 1;
    error.temp = 1;

    
    for (;;) {
        if (error.modem)
            find_modem();

        rc = send_packet(TYPE_PING, NULL, 0);
        if (rc)
            error.modem = 1;
        else {
            debug("ping response: type ??\n");
        }

        _delay_ms(5000);

/*
        if (error.nunchuck)
            nunchuck_init();

        if (error.temp)
            ntemp = search_sensors();

        error.modem = 0;
        error.nunchuck = 0;
        error.temp = 0;

        rc = connect();
        if (rc) {
            error.modem = 1;
            continue;
        }

		if (DS18X20_start_meas(DS18X20_POWER_PARASITE, NULL))
            error.temp = 1;

        _delay_ms(DS18B20_TCONV_12BIT);
        for (i = 0; i < ntemp; i++) {
            if (DS18X20_read(&sensor_ids[i][0], &temp)) {
                error.temp = 1;
                break;
            }

            flags.ack = 0;

            uart_putchar(3);
            uart_putchar(TYPE_TEMP);
            uart_putchar(temp >> 8);
            uart_putchar(temp & 0xff);

            retry = 255;
            while (!flags.ack && --retry) {
                _delay_ms(100);
            }

            if (retry == 0) {
                error.modem = 1;
            }
        }

        rc = nunchuck_read(&n);
        if (rc) {
            error.nunchuck = 1;
        } else {
            flags.ack = 0;

            uart_putchar(sizeof(n) + 1);
            uart_putchar(TYPE_NUNCHUCK);
            for (i = 0; i < sizeof(n); i++)
                uart_putchar(*((uint8_t*)&n + i));

            retry = 255;
            while (!flags.ack && --retry) {
                _delay_ms(100);
            }

            if (retry == 0) {
                error.modem = 1;
            }
        }

        debug("done\n");

        disconnect();

        _delay_ms(10000);
        */
    }
}
