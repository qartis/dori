#include <ctype.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "uart.h"

volatile uint8_t got_packet;
volatile uint8_t packet_buf[8];
volatile uint8_t packet_len;
volatile uint16_t packet_id;

ISR(INT1_vect)
{
    uint8_t i;
    uint8_t canintf = read_register_single(MCP_REGISTER_CANINTF);
    //printf("int! canintf 0x%02x\n", canintf);
    
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

inline int streq(const char *a, const char *b){
    return strcmp(a,b) == 0;
}

int main(void) {
    int buflen = 32;
    char buf[buflen];

	_delay_ms(500);
	uart_init(BAUD(38400));
    puts("system start");

    //PORTD2 power
    DDRD = 0;
    DDRB = 0;
    DDRC = 0;

    PORTD = 0;
    PORTB = 0;
    PORTC = 0;

    for(;;){
        putchar('>');
        putchar(' ');

        fgets(buf,buflen,stdin);
        if (strlen(buf) && buf[strlen(buf)-1] == '\n'){
            buf[strlen(buf)-1] = '\0';
        } else {
            puts("error: buffer full");
        }

        int pin = buf[1] - '0';

        if (0) {
        } else if (streq(buf, "auto")) {
            mode_auto();
        } else if (streq(buf, "manual")) {
            mode_auto();
            LOW(MODE1);
        } else if (streq(buf, "portrait")) {
            mode_auto();
            LOW(MODE2);
        } else if (streq(buf, "video")) {
            mode_auto();
            LOW(MODE3);
        } else if (streq(buf, "night")) {
            mode_auto();
            LOW(MODE4);
        } else if (streq(buf, "program")) {
            mode_auto();
            LOW(MODE6);
        } else if (streq(buf, "av")) {
            mode_auto();
            LOW(MODE1);
            LOW(MODE2);
        } else if (streq(buf, "landscape")) {
            mode_auto();
            LOW(MODE2);
            LOW(MODE4);
        } else if (streq(buf, "stitch")) {
            mode_auto();
            LOW(MODE3);
            LOW(MODE6);
        } else if (streq(buf, "nightsnap")) {
            mode_auto();
            LOW(MODE4);
            LOW(MODE6);
        } else if (streq(buf, "shutterspeed")) {
            mode_auto();
            LOW(MODE1);
            LOW(MODE2);
            LOW(MODE6);
        } else if (streq(buf, "snap")) {
            LOW(SNAP);
            _delay_ms(750);
            HIZ(SNAP);
        } else if (streq(buf, "focus")) {
            LOW(FOCUS);
            _delay_ms(500);
            HIZ(FOCUS);
        } else if (streq(buf, "zi")) {
            LOW(ZOOMIN);
            _delay_ms(50);
            HIZ(ZOOMIN);
        } else if (streq(buf, "zo")) {
            LOW(ZOOMOUT);
            _delay_ms(50);
            HIZ(ZOOMOUT);
        } else if (streq(buf, "up") || streq(buf, "\x1b\x5b\x41")) {
            LOW(UP);
            _delay_ms(200);
            HIZ(UP);
        } else if (streq(buf, "down") || streq(buf, "\x1b\x5b\x42")) {
            LOW(DOWN);
            _delay_ms(200);
            HIZ(DOWN);
        } else if (streq(buf, "left") || streq(buf, "\x1b\x5b\x44")) {
            LOW(LEFT);
            _delay_ms(200);
            HIZ(LEFT);
        } else if (streq(buf, "right") || streq(buf, "\x1b\x5b\x43")) {
            LOW(RIGHT);
            _delay_ms(200);
            HIZ(RIGHT);
        } else if (streq(buf, "disp")) {
            LOW(DISP);
            _delay_ms(200);
            HIZ(DISP);
        } else if (streq(buf, "menu")) {
            LOW(MENU);
            _delay_ms(200);
            HIZ(MENU);
        } else if (streq(buf, "func") || buf[0] == '\0') {
            LOW(FUNC);
            _delay_ms(200);
            HIZ(FUNC);
        } else if (streq(buf, "led")) {
            PINB &= ~(1 << PORTB4);
            if (PINB & PORTB4) puts("led on");
            else puts("led off");
        } else if (streq(buf, "on")) {
            HI(POWER);
            _delay_ms(500);
            HIZ(POWER);
        } else {
            puts("unknown command");
        }
    }
}
