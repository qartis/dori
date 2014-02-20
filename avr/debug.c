#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "debug.h"
#include "irq.h"

volatile uint8_t debug_buf[DEBUG_BUF_SIZE];
volatile uint8_t debug_buf_in;
volatile uint8_t debug_buf_out;

volatile uint8_t debug_tx_ring[DEBUG_TX_BUF_SIZE];
volatile uint8_t debug_tx_ring_in;
volatile uint8_t debug_tx_ring_out;

static FILE mystdout = FDEV_SETUP_STREAM(
    (int (*)(char,FILE*))debug_putchar,
    (int (*)(FILE*))debug_getchar,
    _FDEV_SETUP_RW);

void debug_init(void)
{
    /* 18432000 / 8 = 2304000
       2304000 / 30 = 76800
       so if we overflow every 30 ticks
       that makes 76800 times per second,
       or 2 samples per bit at 38400 baud */

    OCR2A = (30-1);
    TCCR2A = 1 << WGM21;
    TCCR2B = (1 << CS21); /* clk/8 */
    TIMSK2 = (1 << OCIE2A);

    /* tx */
    DDRC |= (1 << PORTC0);
    PORTC |= (1 << PORTC0);

    /* rx */
    DDRC &= ~(1 << PORTC1);
    PORTC |= (1 << PORTC1);

    stdout = &mystdout;
    stdin = &mystdout;
}

void debug_putchar(char c)
{
redo:
    //while ((debug_tx_ring_in + 1 % DEBUG_TX_BUF_SIZE) == debug_tx_ring_out) {}
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        debug_tx_ring[debug_tx_ring_in] = c;
        debug_tx_ring_in = (debug_tx_ring_in + 1);
        debug_tx_ring_in %= DEBUG_TX_BUF_SIZE;
    }

    if (c == '\n') {
        c = '\r';
        goto redo;
    }
}

ISR(TIMER2_COMPA_vect)
{
    static enum {
        INIT,
        STARTBIT1,
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
        if (rxbyte == '\r')
            rxbyte = '\n';

        debug_buf[debug_buf_in] = rxbyte;
        debug_buf_in++;
        debug_buf_in %= DEBUG_TX_BUF_SIZE;

        /* echo */
        debug_putchar(rxbyte);

        if (rxbyte == '\n')
            irq_signal |= IRQ_DEBUG;

        rxstate = INIT;
        break;

    default:
        rxstate++;
    }

    switch (txstate) {
    case INIT:
        if (debug_tx_ring_in == debug_tx_ring_out)
            break;

        PORTC &= ~(1 << PORTC0);
        txstate = STARTBIT1;
        break;

    case BIT0: case BIT1: case BIT2: case BIT3:
    case BIT4: case BIT5: case BIT6: case BIT7:
        if (debug_tx_ring[debug_tx_ring_out] & 1)
            PORTC |= (1 << PORTC0);
        else
            PORTC &= ~(1 << PORTC0);

        debug_tx_ring[debug_tx_ring_out] >>= 1;

        txstate++;
        break;

    case STOPBIT1:
        PORTC |= (1 << PORTC0);
        txstate++;
        break;

    case STOPBIT2:
        debug_tx_ring_out++;
        debug_tx_ring_out %= DEBUG_TX_BUF_SIZE;
        txstate = INIT;
        break;

    default:
        txstate++;
    }
}

uint8_t debug_getchar(void)
{
    uint8_t c;

    while (debug_buf_in == debug_buf_out) {};

    c = debug_buf[debug_buf_out];
    debug_buf_out = (debug_buf_out + 1) % DEBUG_BUF_SIZE;

    return c;
}

void debug_flush(void)
{
    debug_buf_in = 0;
    debug_buf_out = 0;
}
