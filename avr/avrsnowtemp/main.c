#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <string.h>

#include "uart.h"
#include "mcp2515.h"
#include "spi.h"
#include "onewire.h"
#include "ds18x20.h"

enum {
    TYPE_PING = 0xff,
    TYPE_TEMP = 0x02,
    TYPE_NUNCHUCK = 0x03,
};

volatile uint8_t got_packet;
volatile uint8_t packet_buf[8];
volatile uint8_t packet_len;
volatile uint16_t packet_id;

ISR(INT1_vect)
{
    uint8_t i;
    uint8_t canintf = read_register_single(MCP_REGISTER_CANINTF);
//    printf("int! canintf 0x%02x\n", canintf);
    
    if ( canintf & MCP_INTERRUPT_RX1I ) {
        SPI_SELECT();
        spi_write(MCP_COMMAND_READ);
        spi_write(0x71);
        for(i=0;i<13;i++){
            printf("r1 byte %d: %x\n", i, spi_recv());
        }
        SPI_DESELECT();

        printf("READ\n");
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
    uint8_t buf[128];

    uart_init(BAUD(9600));
    _delay_ms(500);
    printf("system start\n");

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
        printf("mcp2515: 0\n");
        _delay_ms(500);
    }

    uint8_t i = 0;
    uint16_t tx_id = 0;
    uint16_t len;
    uint16_t id = 0;
    uint8_t rxlen;
    int16_t temp;
    for (;;) {
        rxlen = 0;

        printf("> ");

        fgets((char *)buf, sizeof(buf), stdin);

        uint8_t ntemp = search_sensors();
        if (ntemp < 1) {
            printf("no sensors found\n");
            continue;
        }

		if (DS18X20_start_meas(DS18X20_POWER_PARASITE, NULL)){
            printf("error start_meas\n");
            continue;
        }

        _delay_ms(DS18B20_TCONV_12BIT);
        buf[rxlen++] = TYPE_TEMP;

        for (i = 0; i < ntemp; i++) {
            if (DS18X20_read_decicelsius(&sensor_ids[i][0], &temp)) {
                printf("error sensor_read\n");
                break;
            }

            printf("read: %x%x (%d)\n", (uint8_t)(temp >> 8), temp & 0xff, temp);

            buf[rxlen++] = temp >> 8;
            buf[rxlen++] = temp & 0xff;
        }



        /*
        cli();
        for (i = 0; i < packet_len; i++) {
            buf[i] = packet_buf[i];
        }
        len = packet_len;
        id = packet_id;
        sei();

        got_packet = 0;

        printf("Received [0x%04x] %db: ", id, len);
        for (i = 0; i < len; i++) {
            printf("%c", buf[i]);
        }
        printf("\n");
        continue;
        */

send:
        load_ff_0(rxlen, tx_id++, buf);

        SPI_SELECT();
        spi_write(0x81);
        SPI_DESELECT();
        rxlen = 0;
    }

    return 0;
}
