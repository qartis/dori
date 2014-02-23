#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "sd.h"
#include "fat.h"
#include "powershot.h"
#include "uart.h"
#include "node.h"
#include "time.h"
#include "spi.h"
#include "irq.h"

#define NUM_LOGS 4
#define LOG_INVALID 0xff
#define LOG_SIZE 512

/*
ISR(INT0_vect) { printf("int\n"); }
ISR(INT1_vect) { printf("int\n"); }
ISR(PCINT0_vect) { printf("int\n"); }
ISR(PCINT1_vect) { printf("int\n"); }
ISR(PCINT2_vect) { printf("int\n"); }
ISR(WDT_vect) { printf("int\n"); }
ISR(TIMER2_COMPA_vect) { printf("int\n"); }
ISR(TIMER2_COMPB_vect) { printf("int\n"); }
ISR(TIMER2_OVF_vect) { printf("int\n"); }
ISR(TIMER1_CAPT_vect) { printf("int\n"); }
ISR(TIMER1_COMPA_vect) { printf("int\n"); }
ISR(TIMER1_COMPB_vect) { printf("int\n"); }
ISR(TIMER1_OVF_vect) { printf("int\n"); }
//ISR(TIMER0_COMPA_vect) { printf("int\n"); }
ISR(TIMER0_COMPB_vect) { printf("int\n"); }
ISR(TIMER0_OVF_vect) { printf("int\n"); }
ISR(SPI_STC_vect) { printf("int\n"); }
ISR(USART_UDRE_vect) { printf("int\n"); }
ISR(USART_TX_vect) { printf("int\n"); }
ISR(ADC_vect) { printf("int\n"); }
ISR(EE_READY_vect) { printf("int\n"); }
ISR(ANALOG_COMP_vect) { printf("int\n"); }
ISR(TWI_vect) { printf("int\n"); }
ISR(SPM_READY_vect) { printf("int\n"); }
*/


inline uint8_t streq(const char *a, const char *b)
{
    return strcmp(a,b) == 0;
}

uint8_t periodic_irq(void)
{
    return 0;
}

uint8_t can_irq(void)
{
    return 0;
}

uint8_t fat_init(void);

void file_read(void)
{
    uint8_t rc;
    uint8_t i;
    printf("read\n");
    spi_init();
    sd_hw_init();

    printf("hey\n");

    _delay_ms(500);

    rc = fat_init();
    DDRD |= (1 << PORTD7);
    PORTD |= (1 << PORTD7);
    if (rc) return;
    printf("hey\n");

    rc = pf_open("tmp");
    printf_P(PSTR("open %u\n"), rc);
    if (rc)
        return;

    uint8_t buf[8];
    uint16_t rd = 1;
    while (rd != 0) {
        rc = pf_read(buf, 8, &rd);
        if (rc) {
            puts_P(PSTR("read er"));
            return;
        }
        for (i = 0; i < rd; i++) {
            printf("%x ", buf[i]);
        }
        printf("\n");
    }
}

void sd_mode(void)
{
    LOW(CS);
    LOW(RELAY);
}

void powershot_mode(void)
{
    HIZ(CS);
    HI(RELAY);
}

void snap(void)
{
    powershot_mode();
    _delay_ms(1000);

    HI(POWER);
    _delay_ms(500);
    HIZ(POWER);

    _delay_ms(2000);

    LOW(SNAP);
    _delay_ms(750);
    HIZ(SNAP);

    _delay_ms(4000);

    HI(POWER);
    _delay_ms(500);
    HIZ(POWER);

    printf("camera off?\n");

    sd_mode();
}

uint8_t uart_irq(void)
{
    char buf[UART_BUF_SIZE];

    printf("got line\n");

    fgets(buf, sizeof(buf), stdin);

    buf[strlen(buf)-1] = '\0';

    //int pin = buf[1] - '0';

    if (buf[0] == 'r' && buf[1] == '\0') {
        printf("got read\n");
        file_read();
    } else if (buf[0] == 's' && buf[1] == '\0') {
        snap();
    }

    return 0;
}

uint8_t fat_init(void)
{
    uint8_t rc;

    sd_init();
    printf("sd init\n");

    rc = disk_initialize();
    printf("dsk %u\n", rc);
    if (rc)
        return rc;

    rc = pf_mount();
    printf("fs %u\n", rc);
    if (rc)
        return rc;

    spi_low();

    return 0;
}

int main(void)
{
    uint8_t rc;

#define MCP2515_PORT PORTB
#define MCP2515_CS PORTB2
#define MCP2515_DDR DDRB

    MCP2515_DDR |= (1 << MCP2515_CS);
    MCP2515_PORT |= (1 << MCP2515_CS);

    /* clear up the SPI bus for the mcp2515 */
    spi_init();
    sd_hw_init();

    //NODE_INIT();
    uart_init(BAUD(UART_BAUD)); 
    _delay_ms(300);

    goto reinit;
reinit:
    reinit = 0;
    puts_P(PSTR("\n" XSTR(MY_ID) " start: " XSTR(VERSION))); 
    puts_P(PSTR(" reboot\n"));

    rc = 0;

    DDRD = 0xFF;
    DDRC = 0xFF;

    sd_mode();

    sei();

    NODE_MAIN();
}
