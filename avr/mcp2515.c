#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include <stdio.h>

#include "mcp2515.h"
#include "spi.h"
#include "time.h"
#include "uart.h"

#define MCP2515_PORT PORTB
#define MCP2515_CS PORTB2
#define MCP2515_DDR DDRB

/* for use by application code */
volatile uint8_t mcp2515_busy;
volatile struct mcp2515_packet_t packet;
volatile uint8_t stfu;
volatile uint8_t expecting_xfer_type;

static volatile uint8_t received_cts;
static volatile uint8_t received_cancel;
static volatile uint8_t received_xfer;

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
        printf_P(PSTR("canstat not correct: %x\n"), read_register(MCP_REGISTER_CANSTAT));
        return 1;
    }

    mcp2515_busy = 0;
    stfu = 0;
    expecting_xfer_type = TYPE_invalid;
    received_xfer = 0;
    received_cts = 0;
    received_cancel = 0;

    return 0;
}

void mcp2515_reset(void)
{
    mcp2515_select();
    spi_write(MCP_COMMAND_RESET);
    mcp2515_unselect();
    _delay_ms(10);
}

uint8_t mcp2515_send(uint8_t type, uint8_t id, uint8_t len, const void *data)
{
    /*
    if (mcp2515_busy) {
        printf_P(PSTR("tx overrun\n"));
        return 1;
    }
    */

    cli();
    load_tx0(type, id, len, (const uint8_t *)data);
    mcp2515_busy = 1;

    modify_register(MCP_REGISTER_CANINTF, MCP_INTERRUPT_TX0I, 0x00);
    mcp2515_select();
    spi_write(MCP_COMMAND_RTS_TX0);
    mcp2515_unselect();
    sei();

    return 0;
}

void load_tx0(uint8_t type, uint8_t id, uint8_t len, const uint8_t *data)
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
        printf_P(PSTR("incorrect len!\n"));
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
        mcp2515_irq_callback();
    } else if (TYPE_XFER(packet.type) && packet.id == MY_ID) {
        /* signal for xfer handler */
        if (packet.type == TYPE_xfer_cts) {
            received_cts = 1;
        } else if (packet.type == TYPE_xfer_cancel) {
            received_cancel = 1;
        } else if (expecting_xfer_type == TYPE_invalid
                || expecting_xfer_type == packet.type) {
            received_xfer = 1;
        }
    } else {
        mcp2515_irq_callback();
        if (packet.type == TYPE_value_periodic && packet.id == ID_time) {
            uint32_t new_time = (uint32_t)packet.data[0] << 24 |
                                (uint32_t)packet.data[1] << 16 |
                                (uint32_t)packet.data[2] << 8  |
                                (uint32_t)packet.data[3] << 0;
            printf_P(PSTR("mcp2515: time set %lu\n"), new_time);
            time_set(new_time);
        } else if (packet.type == TYPE_set_interval && packet.id == MY_ID) {
            periodic_prev = now;
            periodic_interval =  (uint16_t)packet.data[0] << 8 |
                                 (uint16_t)packet.data[1] << 0;

            printf_P(PSTR("period set %u\n"), periodic_interval);
        } else if (packet.type == TYPE_sos_reboot && packet.id == MY_ID) {
            cli();
            wdt_enable(WDTO_15MS);
            for (;;) {};
        }
    }
}

ISR(PCINT0_vect)
{
    uint8_t canintf;

    canintf = read_register(MCP_REGISTER_CANINTF);

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
        printf_P(PSTR("mcp2515 error: %x\n"), read_register(MCP_REGISTER_EFLG));
        modify_register(MCP_REGISTER_CANINTF, MCP_INTERRUPT_ERRI, 0x00);
        modify_register(MCP_REGISTER_EFLG, 0xff, 0x00);
        canintf &= ~(MCP_INTERRUPT_ERRI);
    }

    if (canintf) {
        printf_P(PSTR("ERROR canintf %x\n"), canintf);
        modify_register(MCP_REGISTER_CANINTF, 0xff, 0x00);
    }
}

uint8_t mcp2515_xfer(uint8_t type, uint8_t dest, uint8_t len, void *data)
{
    uint8_t retry;

    received_xfer = 0;
    received_cancel = 0;
    received_cts = 0;

    mcp2515_send(type, dest, len, data);

    retry = 255;
    while (!received_cts && !received_cancel && --retry) {
        _delay_ms(40);
    }

    if (retry == 0) {
        printf_P(PSTR("xfer: cts timeout\n"));
        //didn't get a response
        //expected TYPE_xfer_cts
        return 1;
    } else if (received_cancel) {
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
    received_xfer = 0;
    received_cancel = 0;

    printf_P(PSTR("sent CTS\n"));
    mcp2515_send(TYPE_xfer_cts, sender_id, 0, NULL);

    for (;;) {
        retry = 255;
        while (!received_xfer && !received_cancel && --retry) { _delay_ms(40); }
        printf_P(PSTR("done\n"));
        if (retry == 0) {
            printf_P(PSTR("wt_rx_xf: timeout\n"));
            //timed out waiting for xfer
            return 1;
        }

        if (received_cancel) {
            printf_P(PSTR("wt_rx_xf: cancld\n"));
            return 1;
        }

        rc = xfer_cb();
        if (rc)
            return rc;

        received_xfer = 0;

        mcp2515_send(TYPE_xfer_cts, sender_id, 0, NULL);
        if (packet.len < 8) {
            printf_P(PSTR("rx_xf_wt: success\n"));
            break;
        }
    }

    return 0;
}

uint8_t mcp2515_check_alive(void)
{
    uint8_t rc;

    /* CANSTAT might be all zeroes, so we check CANINTE instead */
    rc = read_register(MCP_REGISTER_CANINTE);

    return (rc == 0b00100111);
}

uint8_t mcp2515_send_xfer_wait(struct mcp2515_packet_t *p)
{
    uint16_t retry;

    expecting_xfer_type = TYPE_invalid;

    printf_P(PSTR("get_packet: looping\n"));
    for (;;) {
        if (!mcp2515_check_alive()) {
            return 1;
        }

        retry = 65535;
        while (--retry > 0) {
            if (received_xfer) {
                *p = packet;
                received_xfer = 0;
                return 0;
            }
            _delay_ms(10);
        }
    }
}
