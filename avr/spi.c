#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#include "spi.h"

void spi_init(void)
{
    DDRB |= (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_CS);
    PORTB |= (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_CS);
    DDRB &= ~(1 << SPI_MISO);

    SPCR = (0 << SPIE) | /* SPI Interrupt Enable */
           (1 << SPE)  | /* SPI Enable */
           (0 << DORD) | /* Data Order: MSB first */
           (1 << MSTR) | /* Master mode */
           (0 << CPOL) | /* Clock Polarity: SCK low when idle */
           (0 << CPHA) | /* Clock Phase: sample on rising SCK edge */
           (0 << SPR1) | /* Clock Frequency: f_OSC / 128 */
           (0 << SPR0);
    spi_low();
}

void spi_low(void)
{
    SPCR |= (1 << SPR1) | (1 << SPR0);
    SPSR &= ~(1 << SPI2X);
}   
   
void spi_medium(void)
{
    spi_high();
    SPSR &= ~(1 << SPI2X);
}   
   
void spi_high(void)
{
    SPCR &= ~((1 << SPR1) | (1 << SPR0));   
    SPSR |= (1 << SPI2X);   
}

#if 0
uint8_t spi_write_timeout(uint8_t data)
{
    uint8_t retry = 0;

    SPDR = data;

    while (!(SPSR & (1<<SPIF)) && --retry){}

    if (!retry) {
        puts_P(PSTR("spi err"));
        return 1;
    }

    return 0;
}
#endif

uint8_t spi_write(uint8_t data)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        SPDR = data;
        while (!(SPSR & (1<<SPIF))) {};
    }
    return SPDR;
}

uint8_t spi_recv(void)
{
    return spi_write(0xff);
}
