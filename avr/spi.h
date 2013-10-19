void spi_init(void);
void spi_low(void);
void spi_medium(void);
void spi_high(void);
uint8_t spi_write(uint8_t);
//uint8_t spi_write_timeout(uint8_t);
uint8_t spi_recv(void);

#if defined(__AVR_ATmega88__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega328P__)
#define SPI_CS   PORTB2
#define SPI_MOSI PORTB3
#define SPI_MISO PORTB4
#define SPI_SCK  PORTB5
#else
#ifdef __AVR_ATmega32__
#define SPI_CS PORTB2
#define SPI_MOSI PORTB5
#define SPI_MISO PORTB6
#define SPI_SCK PORTB7
#endif
#endif

#define SPI_DESELECT() PORTB |=  (1 << SPI_CS)
#define SPI_SELECT()   PORTB &= ~(1 << SPI_CS)
