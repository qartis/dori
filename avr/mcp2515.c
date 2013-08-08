#include <avr/io.h>
#include <util/atomic.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include <stdio.h>

#include "can.h"
#include "mcp2515.h"
#include "spi.h"
#include "time.h"
#include "uart.h"
#include "irq.h"

#define MCP2515_PORT PORTB
#define MCP2515_CS PORTB2
#define MCP2515_DDR DDRB

/* for use by application code */
volatile uint8_t mcp2515_busy;
volatile struct mcp2515_packet_t packet;
volatile uint8_t stfu;

volatile enum XFER_STATE xfer_state;

volatile mcp2515_type_t expecting_xfer_type;

void mcp2515_tophalf(void)
{
    if (irq_signal & IRQ_CAN) {
        //puts_P(PSTR("CAN overrun"));
    }

    irq_signal |= IRQ_CAN;
    packet.unread = 1;

#ifdef CAN_TOPHALF
    can_tophalf();
#endif
}

inline void mcp2515_select(void)
{
    MCP2515_PORT &= ~(1 << MCP2515_CS);
}

inline void mcp2515_unselect(void)
{
    MCP2515_PORT |= (1 << MCP2515_CS);
}

uint8_t read_register(uint8_t address)
{
    uint8_t result;

    mcp2515_select();

    spi_write(MCP_COMMAND_READ);
    spi_write(address);
    result = spi_recv();

    mcp2515_unselect();

    return result;
}

void write_register(uint8_t address, uint8_t value)
{
    mcp2515_select();
    
    spi_write(MCP_COMMAND_WRITE);
    spi_write(address);
    spi_write(value);

    mcp2515_unselect();
}

void modify_register(uint8_t address, uint8_t mask, const uint8_t value)
{
    mcp2515_select();
    
    spi_write(MCP_COMMAND_BITMOD);
    spi_write(address);
    spi_write(mask);
    spi_write(value);

    mcp2515_unselect();
}

uint8_t mcp2515_init(void)
{
    /* mcp2515's clock feed */
    DDRB |= (1 << PORTB1);

    /* setup timer1 to feed mcp2515's clock on OC1A (PB1) */
    TCNT1 = 0;
    OCR1A = 0;
    TCCR1A = (1 << COM1A0); 
    TCCR1B = (1 << CS10) | (1 << WGM12); 

    /* enable mcp2515's interrupt on avr pin pcint0 (pb0) */
    PCICR |= (1 << PCIE0);
    PCMSK0 |= (1 << PCINT0);

    MCP2515_DDR |= (1 << MCP2515_CS);
    mcp2515_unselect();

    mcp2515_reset();

    //--    Set mode to configuration operation.
    modify_register(MCP_REGISTER_CANCTRL, 0xe0, 0x80);

    uint8_t result = read_register(MCP_REGISTER_CANSTAT);
    if ((result & 0xe0) != 0x80) {
        return 1;
    }

    write_register(MCP_REGISTER_CNF1, 0x01);
    write_register(MCP_REGISTER_CNF2, 0x9a);
    write_register(MCP_REGISTER_CNF3, 0x01);

    write_register(MCP_REGISTER_CANINTE, 0b00100111);

    write_register(MCP_REGISTER_RXB0CTRL, 0b01100000);

    modify_register(MCP_REGISTER_CANCTRL, 0xff, 0b00000000);

    uint8_t retry = 10;
    while ((read_register(MCP_REGISTER_CANSTAT) & 0xe0) != 0b00000000 && --retry){};
    if (retry == 0) {
        //printf_P(PSTR("canstat: %x\n"), read_register(MCP_REGISTER_CANSTAT));
        return 1;
    }

    mcp2515_busy = 0;
    stfu = 0;

    return 0;
}

void mcp2515_reset(void)
{
    mcp2515_select();
    spi_write(MCP_COMMAND_RESET);
    mcp2515_unselect();
    _delay_ms(10);
}

uint8_t mcp2515_send2(struct mcp2515_packet_t *p)
{
    return mcp2515_send(p->type, p->id, p->data, p->len);
}

uint8_t mcp2515_send(uint8_t type, uint8_t id, const void *data, uint8_t len)
{
    if (mcp2515_busy) {
        puts_P(PSTR("tx overrun"));
        mcp2515_busy = 0;
        return 1;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        load_tx0(type, id, (const uint8_t *)data, len);
        mcp2515_busy = 1;

        modify_register(MCP_REGISTER_CANINTF, MCP_INTERRUPT_TX0I, 0x00);
        mcp2515_select();
        spi_write(MCP_COMMAND_RTS_TX0);
        mcp2515_unselect();
    }

    return 0;
}

void load_tx0(uint8_t type, uint8_t id, const uint8_t *data, uint8_t len)
{
    uint8_t i;

    id = ((id & 0b00011100) << 3) | (1 << EXIDE) | (id & 0b00000011);

    mcp2515_select();

    spi_write(MCP_COMMAND_LOAD_TX0);
    spi_write(type);
    spi_write(id);
    spi_write(0x00);
    spi_write(0x00);

    spi_write(len);

    for (i = 0; i < len; i++) {
        spi_write(data[i]);
    }

    mcp2515_unselect();
}

void read_packet(uint8_t regnum)
{
    uint8_t i;

    if (packet.unread) {
        //printf_P(PSTR("CAN txovrn x%x"), packet.type);
        /* if we don't return here, then there's a risk of
           the CAN user-mode irq reading a corrupt packet.
           this is disabled to allow important packets to
           be processed while user mode is blocking */
        /* return 0; */
    }

    mcp2515_select();
    spi_write(MCP_COMMAND_READ);
    spi_write(regnum);

    packet.type = spi_recv();
    packet.id = spi_recv();
    packet.more = (packet.id & 0b00010000) >> 4;
    packet.id = ((packet.id & 0b11100000) >> 3) | (packet.id & 0b00000011);

    spi_recv();
    spi_recv();
#if 0
    packet.data[0] = spi_recv(); /* use extended ID bytes to store */
    packet.data[1] = spi_recv(); /* two more data bytes */
#endif

    packet.len = spi_recv() & 0x0f;

    if (packet.len > 8) {
        printf_P(PSTR("mcp len %d!"), packet.len);
        packet.len = 8;
    }

    /* NOTE: this loop starts at i=2, reading into packet_buf[2],
       because we read the first two data bytes already */
    for (i = 0; i < packet.len; i++) {
        packet.data[i] = spi_recv();
    }

    packet.data[i] = '\0';

    mcp2515_unselect();

    if (MY_ID == ID_any) {
        mcp2515_tophalf();
    } else if (packet.type == TYPE_xfer_cancel && packet.id == MY_ID) {
        puts_P(PSTR("ccl"));
        xfer_state = XFER_CANCEL;
        return;
    } else if (packet.type == TYPE_xfer_cts && packet.id == MY_ID) {
        if (xfer_state == XFER_CHUNK_SENT) {
            puts_P(PSTR("cts"));
            xfer_state = XFER_GOT_CTS;
        }
        return;
    } else if (packet.type == TYPE_xfer_chunk) {
        if (xfer_state == XFER_WAIT_CHUNK) {
            puts_P(PSTR("xf_chk"));
            //xfer_got_chunk();
            /* finish this */
            xfer_state = 0;
        }
    } else if (packet.type == TYPE_value_periodic && packet.id == ID_time) {
        uint32_t new_time = (uint32_t)packet.data[0] << 24 |
                            (uint32_t)packet.data[1] << 16 |
                            (uint32_t)packet.data[2] << 8  |
                            (uint32_t)packet.data[3] << 0;
        //printf_P(PSTR("mcp time=%lu\n"), new_time);
        time_set(new_time);
    } else if (packet.type == TYPE_set_interval && packet.id == MY_ID) {
        periodic_prev = now;
        periodic_interval =  (uint16_t)packet.data[0] << 8 |
                                (uint16_t)packet.data[1] << 0;

        //printf_P(PSTR("mcp period=%u\n"), periodic_interval);
    } else if (packet.type == TYPE_sos_reboot && packet.id == MY_ID) {
        cli();
        wdt_enable(WDTO_15MS);
        for (;;) {};
    }

    mcp2515_tophalf();
}

ISR(PCINT0_vect)
{
    uint8_t canintf;

    canintf = read_register(MCP_REGISTER_CANINTF);
    //printf("int! %x\n", canintf);

    canintf &= 0x7f;

    if (canintf & MCP_INTERRUPT_RX0I) {
        read_packet(MCP_REGISTER_RXB0SIDH);
        modify_register(MCP_REGISTER_CANINTF, MCP_INTERRUPT_RX0I, 0x00);
        canintf &= ~(MCP_INTERRUPT_RX0I);
    }

    if (canintf & MCP_INTERRUPT_RX1I) {
        read_packet(MCP_REGISTER_RXB1SIDH);
        modify_register(MCP_REGISTER_CANINTF, MCP_INTERRUPT_RX1I, 0x00);
        canintf &= ~(MCP_INTERRUPT_RX1I);
    }

    if (canintf & MCP_INTERRUPT_TX0I) {
        mcp2515_busy = 0;
        modify_register(MCP_REGISTER_CANINTF, MCP_INTERRUPT_TX0I, 0x00);
        canintf &= ~(MCP_INTERRUPT_TX0I);
    }

    if (canintf & MCP_INTERRUPT_ERRI) {
        uint8_t eflg = read_register(MCP_REGISTER_EFLG);
        printf_P(PSTR("eflg%x\n"), eflg);

        /* TEC > 96 */
        if (eflg & 0b00000100) {
            write_register(MCP_REGISTER_TEC, 0);
        }

        write_register(MCP_REGISTER_CANINTF, 0);
        write_register(MCP_REGISTER_EFLG, 0);

        canintf &= ~(MCP_INTERRUPT_ERRI);
    }

    if (canintf) {
   //     printf_P(PSTR("mcp err intf %x\n"), canintf);
    }

    /* temporary fix: flush all received packets */
    modify_register(MCP_REGISTER_CANINTF, 0xff, 0x00);
}

uint8_t mcp2515_check_alive(void)
{
    uint8_t rc;

    /* CANSTAT might be all zeroes, so we check CANINTE instead */
    rc = read_register(MCP_REGISTER_CANINTE);

    return (rc == 0b00100111);
}

uint8_t mcp2515_xfer(uint8_t type, uint8_t dest, const void *data, uint8_t len)
{
    uint8_t retry;
    uint8_t rc;

    xfer_state = XFER_CHUNK_SENT;

    rc = mcp2515_send(type, dest, data, len);
    if (rc) {
        puts_P(PSTR("xfsd er"));
        return rc;
    }

    retry = 255;
    while (xfer_state == XFER_CHUNK_SENT && --retry)
        _delay_ms(400);

    if (retry == 0) {
        xfer_state = XFER_CANCEL;
        puts_P(PSTR("xfer: cts timeout"));
        //didn't get a response
        //expected TYPE_xfer_cts
        return 1;
    } else if (xfer_state == XFER_CANCEL) {
        return 2;
    }

    return 0;
}

uint8_t mcp2515_receive_xfer_wait(uint8_t type, uint8_t sender_id,
    mcp2515_xfer_callback_t xfer_cb)
{
    uint8_t retry;
    uint8_t rc;

    expecting_xfer_type = type;
    xfer_state = XFER_WAIT_CHUNK;

    puts_P(PSTR("sent CTS"));
    mcp2515_send(TYPE_xfer_cts, sender_id, NULL, 0);

    for (;;) {
        retry = 255;
        while (xfer_state == XFER_WAIT_CHUNK && --retry)
            _delay_ms(40);

        puts_P(PSTR("done"));
        if (retry == 0) {
            /* timed out waiting for xfer */
            xfer_state = XFER_CANCEL;
            puts_P(PSTR("wt_rx_xf: timeout"));
            return 1;
        }

        if (xfer_state == XFER_CANCEL) {
            puts_P(PSTR("wt_rx_xf: cancelled"));
            return 1;
        }

        /* handle this new chunk */
        rc = xfer_cb();
        if (rc) {
            puts_P(PSTR("xfer_cb error, cancelled"));
            xfer_state = XFER_CANCEL;
            return rc;
        }

        xfer_state = XFER_WAIT_CHUNK;

        mcp2515_send(TYPE_xfer_cts, sender_id, NULL, 0);

        if (packet.len < 8) {
            /* we just sent our last (partial) chunk. success. */
            puts_P(PSTR("rx_xf_wt: success"));
            break;
        }
    }

    return 0;
}
