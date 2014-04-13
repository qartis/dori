#include <avr/io.h>
#include <util/atomic.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <avr/power.h>

#include "can.h"
#include "mcp2515.h"
#include "spi.h"
#include "time.h"
#include "uart.h"
#include "irq.h"
#include "errno.h"

#define MCP2515_PORT PORTB
#define MCP2515_CS PORTB2
#define MCP2515_DDR DDRB

/* for use by application code */
volatile struct mcp2515_packet_t packet;

volatile uint8_t mcp2515_busy;
volatile uint8_t stfu;
volatile enum XFER_STATE xfer_state;

void mcp2515_tophalf(void)
{
    if (irq_signal & IRQ_CAN) {
        puts_P(PSTR("CAN overrun"));
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

void mcp2515_dump(void)
{
	uint8_t tec, rec, eflg;
    uint8_t intf;

    tec = read_register(MCP_REGISTER_TEC);
    rec = read_register(MCP_REGISTER_REC);
    eflg = read_register(MCP_REGISTER_EFLG);

    intf = read_register(MCP_REGISTER_CANINTF);

	printf_P(PSTR("TEC:x%x\n"), tec);
	printf_P(PSTR("REC:x%x\n"), rec);
	printf_P(PSTR("EFLG:x%x\n"), eflg);
	printf_P(PSTR("INTF:x%x\n"), intf);

	if ((rec > 127) || (tec > 127))
		puts_P(PSTR("Error: Passive or Bus-Off"));

	if (eflg & MCP_EFLG_RX1OVR)
		puts_P(PSTR("Receive Buffer 1 Overflow"));
	if (eflg & MCP_EFLG_RX0OVR)
		puts_P(PSTR("Receive Buffer 0 Overflow"));
	if (eflg & MCP_EFLG_TXBO)
		puts_P(PSTR("Bus-Off"));
	if (eflg & MCP_EFLG_TXEP)
		puts_P(PSTR("Receive Error Passive"));
	if (eflg & MCP_EFLG_TXWAR)
		puts_P(PSTR("Transmit Error Warning"));
	if (eflg & MCP_EFLG_RXWAR)
		puts_P(PSTR("Receive Error Warning"));
	if (eflg & MCP_EFLG_EWARN )
		puts_P(PSTR("Receive Error Warning"));

    write_register(MCP_REGISTER_TEC, 0);
    write_register(MCP_REGISTER_REC, 0);
    write_register(MCP_REGISTER_EFLG, 0);
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
    uint8_t rc;
    uint8_t retry;

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

    rc = read_register(MCP_REGISTER_CANSTAT);
    if ((rc & 0xe0) != 0x80) {
        return ERR_MCP_HW;
    }

#if (F_CPU == 8000000L)
    write_register(MCP_REGISTER_CNF1, 0x01);
    write_register(MCP_REGISTER_CNF2, 0x9a);
    write_register(MCP_REGISTER_CNF3, 0x01);
#elif (F_CPU == 18432000L)
    write_register(MCP_REGISTER_CNF1, 0x01);
    write_register(MCP_REGISTER_CNF2, 0xb6);
    write_register(MCP_REGISTER_CNF3, 0x07);
#else
#error no known MCP2515 values for this clock speed
#endif

    write_register(MCP_REGISTER_CANINTE, 0b00100111);

    write_register(MCP_REGISTER_RXB0CTRL, 0b01100000);

    modify_register(MCP_REGISTER_CANCTRL, 0xff, 0b00000000);

    retry = 10;
    while ((read_register(MCP_REGISTER_CANSTAT) & 0xe0) != 0b00000000 &&
            --retry) {};

    if (retry == 0) {
        return ERR_MCP_HW;
    }

    mcp2515_busy = 0;

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
    return mcp2515_send_sensor(p->type, p->id, p->sensor, p->data, p->len);
}

uint8_t mcp2515_sendpacket_wait(struct mcp2515_packet_t *p)
{
    return mcp2515_send_wait(p->type, p->id, p->sensor, p->data, p->len);
}

uint8_t mcp2515_send(uint8_t type, uint8_t id, const void *data, uint8_t len)
{
    return mcp2515_send_sensor(type, id, 0, data, len);
}

uint8_t mcp2515_send_wait(uint8_t type, uint8_t id, uint16_t sensor, const void *data, uint8_t len)
{
    uint8_t retry;

    retry = 255;
    while (mcp2515_busy && --retry) {
        _delay_ms(1);
    }

    return mcp2515_send_sensor(type, id, sensor, data, len);
}

void mcp2515_handle_packet(uint8_t type, uint8_t id, uint16_t sensor, const volatile uint8_t *data, uint8_t len)
{
    uint16_t new_periodic_interval;

    if (type == TYPE_set_interval && (id == MY_ID || id == ID_any)) {
        periodic_prev = now;
        new_periodic_interval =
            (uint16_t)data[0] << 8 |
            (uint16_t)data[1] << 0;

        if (new_periodic_interval > 5)
            periodic_interval = new_periodic_interval;
    } else if (type == TYPE_sos_reboot && (id == MY_ID || id == ID_any)) {
        cli();
        wdt_enable(WDTO_15MS);
        for (;;) {};
    } else if (type == TYPE_sos_stfu && (id == MY_ID || id == ID_any)) {
        stfu = 1;
    } else if (type == TYPE_sos_nostfu && (id == MY_ID || id == ID_any)) {
        stfu = 0;
    }
}

uint8_t mcp2515_send_sensor(uint8_t type, uint8_t id, uint16_t sensor, const uint8_t *data, uint8_t len)
{
    mcp2515_handle_packet(type, id, sensor, data, len);

    if (stfu) {
        return 0;
    }

    if (mcp2515_busy) {
        mcp2515_init();
        return ERR_MCP_HW;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        load_tx0(type, id, sensor, (const uint8_t *)data, len);
        mcp2515_busy = 1;

        modify_register(MCP_REGISTER_CANINTF, MCP_INTERRUPT_TX0I, 0x00);
        mcp2515_select();
        spi_write(MCP_COMMAND_RTS_TX0);
        mcp2515_unselect();
    }

    return 0;
}

void load_tx0(uint8_t type, uint8_t id, uint16_t sensor, const uint8_t *data, uint8_t len)
{
    uint8_t i;

    id = ((id & 0b00011100) << 3) | (1 << EXIDE) | (id & 0b00000011);

    mcp2515_select();

    spi_write(MCP_COMMAND_LOAD_TX0);
    spi_write(type);
    spi_write(id);
    spi_write(sensor >> 8);
    spi_write(sensor & 0xFF);

    if (len > 8) {
        len = 8;
    }

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

    packet.sensor = spi_recv();
    packet.sensor <<= 8;
    packet.sensor |= spi_recv();

    packet.len = spi_recv() & 0x0f;

    if (packet.len > 8) {
        packet.len = 8;
    }

    for (i = 0; i < packet.len; i++) {
        packet.data[i] = spi_recv();
    }

    packet.data[i] = '\0';

    mcp2515_unselect();

    if ((packet.type == TYPE_value_explicit ||
        packet.type == TYPE_value_periodic) &&
        packet.sensor == SENSOR_time) {
        uint32_t new_time =
            (uint32_t)packet.data[0] << 24 |
            (uint32_t)packet.data[1] << 16 |
            (uint32_t)packet.data[2] << 8  |
            (uint32_t)packet.data[3] << 0;
        time_set(new_time);
    }

    if (MY_ID == ID_any) {
        mcp2515_tophalf();
        return;
    }

    // Should both modems handle this??
    if (packet.type == TYPE_xfer_cancel && packet.id == MY_ID) {
        puts_P(PSTR("ccl"));
        xfer_state = XFER_CANCEL;
        return;
    } else if (packet.type == TYPE_xfer_cts && packet.id == MY_ID) {
        if (xfer_state == XFER_CHUNK_SENT) {
            puts_P(PSTR("cts"));
            xfer_state = XFER_GOT_CTS;
        }
        return;
    }

    if (MY_ID == ID_modema || MY_ID == ID_modemb) {
        mcp2515_tophalf();
    }

    mcp2515_handle_packet(packet.type, packet.id, packet.sensor, packet.data, packet.len);

    if (packet.id == MY_ID) {
        mcp2515_tophalf();
    }
}

ISR(PCINT0_vect)
{
    uint8_t canintf;

    canintf = read_register(MCP_REGISTER_CANINTF);
    //printf("int! %x\n", canintf);

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

    if (canintf & MCP_INTERRUPT_MERR) {
        puts_P(PSTR("mcp: MERR"));
        modify_register(MCP_REGISTER_CANINTF, MCP_INTERRUPT_MERR, 0x00);
    }
}

void mcp2515_xfer_begin(void)
{
    xfer_state = XFER_INIT;
}

uint8_t mcp2515_xfer(uint8_t type, uint8_t dest, uint16_t sensor, const void *data, uint8_t len)
{
    uint8_t retry;
    uint8_t rc;
    uint8_t i;


    printf("state %d\n", xfer_state);
    _delay_ms(500);

    for (i = 0; i < 3; i++) {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            if (xfer_state == XFER_CANCEL) {
                printf("CANCELLED\n");
                _delay_ms(20);
                return ERR_MCP_XFER_CANCEL;
            }
            xfer_state = XFER_CHUNK_SENT;
        }

        rc = mcp2515_send_wait(type, dest, sensor, data, len);
        if (rc) {
            puts_P(PSTR("xfsd er"));
            return rc;
        }

        /* if we resend this packet, this will flag it as a duplicate */
        dest = ID_none;

        /* TODO change this timeout when DORI is launched */
        /* 255 * 80ms = 20400ms */
        retry = 255;
        while (xfer_state == XFER_CHUNK_SENT && --retry) {
            _delay_ms(80);
        }

        if (retry == 0) {
            /* didn't get a cts, so we retry */
            puts_P(PSTR("xfer: cts timeout"));
        } else if (xfer_state == XFER_CANCEL) {
            return ERR_MCP_XFER_CANCEL;
        } else {
            break;
        }
    }

    if (retry == 0) {
        return ERR_MCP_XFER_TIMEOUT;
    }

    return 0;
}
