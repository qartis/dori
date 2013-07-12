#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>

#include "sram.h"
#include "spi.h"

#define SRAM_MODE 0x41 /* sequential access, hold pin disabled */

#define SRAM_SELECT()   PORTD &= ~(1 << PORTD6);
#define SRAM_DESELECT() PORTD |=  (1 << PORTD6);

enum {
    SRAM_WRITE_STATUS = 1,
    SRAM_WRITE = 2,
    SRAM_READ = 3,
    SRAM_READ_STATUS = 5,
};

void sram_hw_init(void)
{
    DDRD |= (1 << PORTD6);
    SRAM_DESELECT();
}

uint8_t sram_read_status(void)
{
    uint8_t val;

    SRAM_SELECT();
    spi_write(SRAM_READ_STATUS);
    val = spi_recv();
    SRAM_DESELECT();

    return val;
}

uint8_t sram_init(void)
{
    uint8_t retry;

    SRAM_DESELECT();

    SRAM_SELECT();
    spi_write(SRAM_WRITE_STATUS);
    spi_write(0x41);
    SRAM_DESELECT();

    retry = 255;
    while (sram_read_status() != SRAM_MODE && --retry) {
        printf("sram problem\n");
        SRAM_SELECT();
        spi_write(SRAM_WRITE_STATUS);
        spi_write(SRAM_MODE);
        SRAM_DESELECT();
    }

    return retry != 0;
}

void sram_write(uint16_t addr, uint8_t val)
{
    SRAM_SELECT();
    spi_write(SRAM_WRITE);
    spi_write((addr >> 8) & 0xff);
    spi_write(addr & 0xff);
    spi_write(val);
    SRAM_DESELECT();
}

void sram_read_begin(uint16_t addr)
{
    SRAM_SELECT();
    spi_write(SRAM_READ);
    spi_write((addr >> 8) & 0xff);
    spi_write(addr & 0xff);
}

uint8_t sram_read_byte(void)
{
    return spi_recv();
}

void sram_read_end(void)
{
    SRAM_DESELECT();
}
