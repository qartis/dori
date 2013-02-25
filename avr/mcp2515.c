#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>

#include "mcp2515.h"
#include "spi.h"

#define MCP2515_PORT PORTB
#define MCP2515_CS PORTB2
#define MCP2515_DDR DDRB

/* for use by application code */
volatile uint8_t mcp2515_busy;
volatile struct mcp2515_packet_t packet;
volatile uint8_t stfu;
volatile uint8_t received_xfer;
volatile uint8_t got_packet;

static volatile uint8_t received_cts;
static volatile uint8_t received_cancel;

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

    /* enable mcp2515's interrupt on avr pin int1 (PD3 for 88p) */
#ifndef __AVR_ATmega328P__
#warning this code only works on 328p
#endif
    EICRA = (1 << ISC11);
    EIMSK |= (1 << INT1);

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

    write_register(MCP_REGISTER_CANINTE, 0x7f);

    write_register(MCP_REGISTER_RXB0CTRL, 0b01100000);

    modify_register(MCP_REGISTER_CANCTRL, 0xff, 0b00000000);

    uint8_t retry = 10;
    while ((read_register(MCP_REGISTER_CANSTAT) & 0xe0) != 0b00000000 && --retry){};
    if (retry == 0) {
        printf("canstat not correct: %x\n", read_register(MCP_REGISTER_CANSTAT));
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

uint8_t mcp2515_send(uint8_t type, uint8_t id, uint8_t len, const uint8_t *data)
{
    if (mcp2515_busy) {
        printf("tx overrun\n");
        return 1;
    }

    cli();
    load_ff_0(type, id, len, data);
    mcp2515_busy = 1;

    modify_register(MCP_REGISTER_CANINTF, MCP_INTERRUPT_TX0I, 0x00);
    mcp2515_select();
    spi_write(MCP_COMMAND_RTS_TX0);
    mcp2515_unselect();
    sei();

    return 0;
}

void load_ff_0(uint8_t type, uint8_t id, uint8_t len, const uint8_t *data)
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
        printf("incorrect len!\n");
        packet.len = 8;
    }

    /* NOTE: this loop starts at i=2, reading into packet_buf[2],
       because we read the first two data bytes already */
    for (i = 0; i < packet.len; i++) {
        packet.data[i] = spi_recv();
    }

    packet.data[i] = '\0';

    mcp2515_unselect();

    if (packet.type == TYPE_XFER_CTS && packet.id == MY_ID)
        received_cts = 1;
    else if (packet.type == TYPE_XFER_CANCEL && packet.id == MY_ID)
        received_cancel = 1;
    else
        mcp2515_callback();

    got_packet = 1;
}

ISR(INT1_vect)
{
    uint8_t canintf;

    canintf = read_register(MCP_REGISTER_CANINTF);

    printf("int %x\n", canintf);

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

    if (canintf) {
        printf("ERROR canintf %x\n", canintf);
        modify_register(MCP_REGISTER_CANINTF, 0xff, 0x00);
    }
}

uint8_t mcp2515_xfer(uint8_t type, uint8_t dest, uint8_t len, uint8_t *data)
{
    uint8_t retry;

    received_cts = 0;
    received_cancel = 0;
    mcp2515_send(type, dest, len, data);

    retry = 255;
    while (!received_cts && !received_cancel && --retry) { _delay_ms(40); }
    if (retry == 0) {
        printf("begin_xfer: timeout waiting for cts\n");
        //didn't get a response
        //expected TYPE_XFER_CTS
        return 1;
    } else if (received_cancel) {
        return 2;
    }

    return 0;
}

uint8_t mcp2515_wait_receive_transfer(uint8_t type)
{
    uint8_t retry;

    for (;;) {
        retry = 255;
        while (!received_xfer && --retry) { _delay_ms(40); }
        if (retry == 0) {
            printf("wait_rx_xfer: timeout waiting for packet\n");
            //timed out waiting for xfer
            return 1;
        }
        //packet.more stores whether there will be more or not
        mcp2515_callback();
        mcp2515_send(TYPE_XFER_CTS, packet.id, 0, NULL);
        if (!packet.more) {
            printf("wait_rx_xfer: done xfer\n");
            break;
        }
    }

    return 0;
    //done receiving transfer body, user code has finished handling it
}

struct mcp2515_packet_t mcp2515_get_packet(void)
{
    got_packet = 0;

    printf("get_packet: looping\n");
    while (got_packet == 0) {
        _delay_ms(10);
    }
    printf("get_packet: done\n");

    return packet;
}

