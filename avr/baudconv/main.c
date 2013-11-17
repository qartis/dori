#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>

#define BAUD(rate) (F_CPU / (rate * 16L) - 1)

/* bytes to be output on the slow (software uart) side */
volatile uint8_t slow_ring[256];
volatile uint8_t slow_ring_in;
volatile uint8_t slow_ring_out;

/* bytes to be output on the fast (hardware uart) side */
volatile uint8_t fast_ring[256];
volatile uint8_t fast_ring_in;
volatile uint8_t fast_ring_out;

void uart_init(uint16_t ubrr)
{
    UBRR0H = ubrr >> 8;  
    UBRR0L = ubrr;  

    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0) | (1 << TXCIE0);
}

void slow_putchar(char c)
{
    slow_ring[slow_ring_in] = c;
    slow_ring_in++;
}

void fast_putchar(uint8_t c)
{
    if ((UCSR0A & (1 << UDRE0))) {
        UDR0 = c;
    } else {
        fast_ring[fast_ring_in] = c;
        fast_ring_in = (fast_ring_in + 1);
    }
}

ISR(TIMER0_COMPA_vect)
{
    static enum {
        INIT,
        STARTBIT1,
        STARTBIT2,
        BIT0, BIT0_WAIT,
        BIT1, BIT1_WAIT,
        BIT2, BIT2_WAIT,
        BIT3, BIT3_WAIT,
        BIT4, BIT4_WAIT,
        BIT5, BIT5_WAIT,
        BIT6, BIT6_WAIT,
        BIT7, BIT7_WAIT,
        STOPBIT1,
        STOPBIT2,
    } rxstate, txstate;
    static uint8_t rxbyte;

    switch (rxstate) {
    case INIT:
        if ((PINC & (1 << PINC1)))
            break;

        rxstate = STARTBIT1;
        rxbyte = 0;
        break;

    case STARTBIT1:
        /* verify that the start bit is LOW for at least half
           a bit period. if not, then it was just noise and we
           go back to INIT */
        if ((PINC & (1 << PINC1))) {
            rxstate = INIT;
        } else {
            rxstate++;
        }

        break;

    case BIT0: case BIT1: case BIT2: case BIT3:
    case BIT4: case BIT5: case BIT6: case BIT7:
        rxbyte >>= 1;
        if (PINC & (1 << PINC1))
            rxbyte |= (1 << 7);

        rxstate++;
        break;

    case STOPBIT2:
        fast_putchar(rxbyte);
        rxstate = INIT;
        break;

    default:
        rxstate++;
    }

    switch (txstate) {
    case INIT:
        if (slow_ring_in == slow_ring_out)
            break;

        PORTC &= ~(1 << PORTC0);
        txstate++;

    case BIT0: case BIT1: case BIT2: case BIT3:
    case BIT4: case BIT5: case BIT6: case BIT7:
        if (slow_ring[slow_ring_out] & 1)
            PORTC |= (1 << PORTC0);
        else
            PORTC &= ~(1 << PORTC0);

        slow_ring[slow_ring_out] >>= 1;

        txstate++;
        break;

    case STOPBIT2:
        slow_ring_out++;
        txstate = INIT;
        break;

    default:
        txstate++;
    }
}

ISR(USART_RX_vect)
{
    uint8_t c = UDR0;
    slow_putchar(c);
}

ISR(USART_TX_vect)
{
    uint8_t c;

    if (fast_ring_in != fast_ring_out) {
        c = fast_ring[fast_ring_out];
        fast_ring_out = (fast_ring_out + 1);
        UDR0 = c;
    }
}
 
void main(void)
{
    /* 18432000 / 8 = 2304000
       2304000 / 30 = 76800
       so if we overflow every 30 ticks
       that makes 76800 times per second,
       or 2 samples per bit at 38400 baud */

    OCR0A = 30;
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01); /* clk/8 */
    TIMSK0 = (1 << OCIE0A);

    /* tx */
    DDRC |= (1 << PORTC0);
    PORTC |= (1 << PORTC0);

    /* rx */
    DDRC &= ~(1 << PORTC1);
    PORTC |= (1 << PORTC1);

    uart_init(BAUD(9600));

    sei();

    for (;;) {
    }
}
