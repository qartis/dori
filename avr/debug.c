#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "debug.h"
#include "irq.h"

/* NOTE: debug's interrupt fires very often, and will
   slow down any loop-based delays by as much as 2x.
   be careful with this */

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

inline void disable_debug_timer(void)
{
    TCCR2B = 0;
    PCMSK1 |= (1 << PCINT9);
}

inline void enable_debug_timer(void)
{
    TCCR2B = (1 << CS21); /* clk/8 */
    PCMSK1 &= ~(1 << PCINT9);
    TCNT2 = 0;
}

ISR(PCINT1_vect)
{
    /* turn on timer2, which will capture this
       incoming byte. also disable pcint9 while
       we're recieving the byte */
    enable_debug_timer();
}

void debug_init(void)
{
    /* 18432000 / 8 = 2304000
       2304000 / 30 = 76800
       so if we overflow every 30 ticks
       that makes 76800 times per second,
       or 2 samples per bit at 38400 baud */
    OCR2A = (30-1);
    TCCR2A = 1 << WGM21;
    TIMSK2 = (1 << OCIE2A);

    PCICR |= (1 << PCIE1);

    /* detect activity on debug rx pin */
    disable_debug_timer();

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
    uint8_t next_idx;

again:
    next_idx = (debug_tx_ring_in + 1) % DEBUG_TX_BUF_SIZE;
    if (next_idx == debug_tx_ring_out) {
        return;
    }

    debug_tx_ring[debug_tx_ring_in] = c;
    debug_tx_ring_in = next_idx;

    if (c == '\n') {
        c = '\r';
        goto redo;
    }
}

ISR(TIMER2_COMPA_vect)
{
    static uint8_t rxbyte;
    static enum {
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

    switch (rxstate) {
    case STARTBIT1:
        if ((PINC & (1 << PINC1)))
            break;

        rxstate = STARTBIT2;
        rxbyte = 0;
        break;

    case STARTBIT2:
        /* verify that the start bit is LOW for at least half
           a bit period. if not, then it was just noise and we
           go back to STARTBIT1 */
        if ((PINC & (1 << PINC1))) {
            rxstate = STARTBIT1;
        } else {
            rxstate = BIT0;
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

        rxstate = STARTBIT1;
        break;

    default:
        rxstate++;
    }

    switch (txstate) {
    case STARTBIT1:
        if (debug_tx_ring_in == debug_tx_ring_out)
            break;

        PORTC &= ~(1 << PORTC0);
        txstate = STARTBIT2;
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
        txstate = STARTBIT1;
        break;

    default:
        txstate++;
    }

    if (rxstate == STARTBIT1 && txstate == STARTBIT1 &&
            debug_tx_ring_in == debug_tx_ring_out) {
        disable_debug_timer();
    }
}

uint8_t debug_getchar(void)
{
    uint8_t c;

    if (debug_buf_in == debug_buf_out) {
        return 0;
    }

    c = debug_buf[debug_buf_out];
    debug_buf_out = (debug_buf_out + 1) % DEBUG_BUF_SIZE;

    return c;
}

void debug_flush(void)
{
    debug_buf_in = 0;
    debug_buf_out = 0;
}
