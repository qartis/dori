#include <stdlib.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "uart.h"
#include "modem.h"
#include "debug.h"
#include "mcp2515.h"
#include "spi.h"

volatile uint8_t got_packet;
volatile uint8_t packet_buf[8];
volatile uint8_t packet_len;
volatile uint16_t packet_id;

ISR(INT1_vect)
{
    uint8_t i;
    uint8_t canintf = read_register_single(MCP_REGISTER_CANINTF);
    //debugf("int! canintf 0x%02x\n", canintf);
    
    if ( canintf & MCP_INTERRUPT_RX1I ) {
        SPI_SELECT();
        spi_write(MCP_COMMAND_READ);
        spi_write(0x71);
        for(i=0;i<13;i++){
            //printf("r1 byte %d: %x\n", i, spi_recv());
        }
        SPI_DESELECT();

        //printf("READ\n");
        modify_register(MCP_REGISTER_CANINTF, MCP_INTERRUPT_RX1I, 0x00 );
    }

    if (canintf & MCP_INTERRUPT_RX0I) {
        SPI_SELECT();
        spi_write(MCP_COMMAND_READ);
        spi_write(0x61);

        packet_id = spi_recv() << 3;
        packet_id |= spi_recv() >> 5;

        spi_recv(); /* ignore extended ID */
        spi_recv();

        packet_len = spi_recv();

        for (i = 0; i < packet_len; i++) {
            packet_buf[i] = spi_recv();
            //printf("r0 byte %d: %x\n", i, packet_buf[i]);
        }

        SPI_DESELECT();

        got_packet = 1;
        //printf("READ\n");
        modify_register( MCP_REGISTER_CANINTF, MCP_INTERRUPT_RX0I, 0x00 );
    }
    
    if (canintf & MCP_INTERRUPT_TX0I) {
//        printf("DISABLE\n");
        modify_register( MCP_REGISTER_CANINTE, MCP_INTERRUPT_TX0I, 0x00 );
    }
}

int main(void)
{
    uint8_t i;
    uint8_t rc;

    struct {
        uint8_t modem : 1;
        uint8_t nunchuck : 1;
        uint8_t temp : 1;
    } error;

    //i2c_init(I2C_FREQ(400000));
	uart_init(BAUD(9600));

    _delay_ms(500);

    debug_init();
    debug("init done\n");

    DDRB |= (1 << PORTB1);

    TCCR1A = (1 << COM1A0); 
    TCCR1B = (1 << CS10) | (1 << WGM12); 

    TCNT1 = 0;
    OCR1A = 0;

#ifdef __AVR_ATmega32__
    MCUCR = (1 << ISC11);
    GICR |= (1 << INT1);
#else
    EICRA = (1 << ISC11);
    EIMSK |= (1 << INT1);
#endif

    sei();

    while (mcp2515_init() == 0) {
        debug("mcp2515: 0\n");
        _delay_ms(500);
    }


    error.modem = 1;
    error.nunchuck = 1;
    error.temp = 1;

    
    for (;;) {
        while (!got_packet) {
            _delay_ms(50);
        }

        find_modem();

        uint8_t buf[128];
        uint16_t len;
        uint16_t id = 0;


        ATOMIC_BLOCK(ATOMIC_FORCEON) {
            for (i = 0; i < packet_len; i++) {
                buf[i] = packet_buf[i];
                debugf("buf[%d] = %x\n", i, buf[i]);
            }
            len = packet_len;
            id = packet_id;

            got_packet = 0;
        }

        debugf("sending packet size %d\n", packet_len);
        rc = send_packet(TYPE_TEMP, buf, packet_len);
        if (rc){
            debugf("send packet error: %d\n", rc);
        }
        else {
            debugf("ping response: type %d\n", rc);
        }

/*
        printf("Received [0x%04x] %db: ", id, len);
        for (i = 0; i < len; i++) {
            printf("%c", buf[i]);
        }
        printf("\n");
        continue;




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
