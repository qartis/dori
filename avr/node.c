#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "node.h"
#include "uart.h"
#include "debug.h"
#include "time.h"
#include "spi.h"
#include "mcp2515.h"
#include "irq.h"

volatile uint8_t reinit;
struct mcp2515_packet_t packet __attribute__ ((section (".noinit")));
uint8_t boot_mcusr __attribute__ ((section (".noinit")));

void get_mcusr(void) __attribute__((section(".init3"), naked, used));
void get_mcusr(void)
{
    boot_mcusr = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

ISR(BADISR_vect)
{
    uart_tx_empty();
    for (;;) {
        printwait_P(PSTR("BADISR\n"));
    }
}

void node_init(void)
{
    _delay_ms(300);
    uart_init(BAUD(UART_BAUD));
    DEBUG_INIT;
    time_init();
    spi_init();
    set_sleep_mode(SLEEP_MODE_IDLE);

    _delay_ms(300);
    printf_P(PSTR("\n" XSTR(MY_ID) " start: " XSTR(VERSION) " "));
    printf_P((boot_mcusr & (1 << WDRF)) ? PSTR("wd") : PSTR(""));
    printf_P((boot_mcusr & (1 << BORF)) ? PSTR("bo") : PSTR(""));
    printf_P((boot_mcusr & (1 << EXTRF)) ? PSTR("ext") : PSTR(""));
    printf_P((boot_mcusr & (1 << PORF)) ? PSTR("po") : PSTR(""));
    printf_P((boot_mcusr & ((1 << PORF) | (1 << EXTRF) | (1 << BORF) | (1 << WDRF))) == 0 ? PSTR("software") : PSTR(""));
    printf_P(PSTR(" reboot\n"));

    goto reinit;

reinit:
    //uart_tx_flush();
    _delay_ms(1000);
    cli();
    reinit = 0;
    while (mcp2515_init()) {
        printwait_P(PSTR("mcp: init\n"));
        _delay_ms(1500);
    }
}

uint8_t node_debug_common(const char *cmd)
{
    uint8_t rc;

    if (strcmp(cmd, "mcp") == 0) {
        mcp2515_dump();
        return 0;
    } else if (strcmp(cmd, "mcp_reinit") == 0) {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            rc = mcp2515_init();
        }

        if (rc != 0) {
            puts_P(PSTR("!mcpinit"));
        }
        return 0;
    } else if (strcmp(cmd, "uptime") == 0) {
        printf_P(PSTR("%lds\n"), uptime);
        return 0;
    } else if (strcmp(cmd, "reboot") == 0) {
        cli();
        wdt_enable(WDTO_15MS);
        for (;;) {};
    }

    return 1;
}

uint8_t node_main(void)
{
    /*wdt_enable(WDTO_8S);*/
    uint8_t rc;

    rc = 0;

    PROMPT

    for (;;) {
        if (0 && reinit) return rc;

        while (irq_signal == 0) { /*sleep();*/ }

        CHECK_USER1;
        CHECK_USER2;
        CHECK_USER3;

        if (irq_signal & IRQ_CAN) {
            rc = can_irq();
            irq_signal &= ~IRQ_CAN;
            if (rc) {
                printwait_P(PSTR("$$c\n"));
                return rc;
            }
        }

        if (irq_signal & IRQ_PERIODIC) {
            rc = periodic_irq();
            irq_signal &= ~IRQ_PERIODIC;
            if (rc) {
                printwait_P(PSTR("$$p\n"));
                return rc;
            }
        }

        CHECK_UART;
        CHECK_DEBUG;
    }
}
