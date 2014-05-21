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
#include "ring.h"


#define MCP2515_PORT PORTB
#define MCP2515_CS PORTB2
#define MCP2515_DDR DDRB

#define TRIGGER_CAN_IRQ() do { irq_signal |= IRQ_CAN; } while(0)

/* for use by application code */
volatile uint8_t mcp2515_busy;
volatile uint8_t stfu;
volatile enum XFER_STATE xfer_state;

volatile uint8_t mcp_ring_array[64];
volatile struct ring_t mcp_ring;
uint32_t busy_since;

inline void mcp2515_select(void)
{
    MCP2515_PORT &= ~(1 << MCP2515_CS);
}

inline void mcp2515_unselect(void)
{
    MCP2515_PORT |= (1 << MCP2515_CS);
}

uint8_t mcp2515_get_packet(struct mcp2515_packet_t *packet)
{
    uint8_t i;

    if (ring_bytes(&mcp_ring) < CAN_HEADER_LEN) {
        return 1;
    }

    if (ring_bytes(&mcp_ring) < CAN_HEADER_LEN + ring_get_idx(&mcp_ring, 4)) {
        return 1;
    }

    packet->type = ring_get(&mcp_ring);
    packet->id = ring_get(&mcp_ring);
    packet->sensor = ring_get(&mcp_ring);
    packet->sensor <<= 8;
    packet->sensor |= ring_get(&mcp_ring);
    packet->len = ring_get(&mcp_ring);

    for (i = 0; i < packet->len; i++) {
        packet->data[i] = ring_get(&mcp_ring);
    }

    return 0;
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

    mcp_ring = ring_init(mcp_ring_array, sizeof(mcp_ring_array));

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

uint8_t mcp2515_handle_sos(uint8_t type, uint8_t id)
{
    if (type == TYPE_sos_reboot && (id == MY_ID || id == ID_any)) {
        cli();
        wdt_enable(WDTO_15MS);
        for (;;) {};
    } else if (type == TYPE_sos_stfu && (id == MY_ID || id == ID_any)) {
        stfu = 1;
    } else if (type == TYPE_sos_nostfu && (id == MY_ID || id == ID_any)) {
        stfu = 0;
    } else {
        return 1;
    }

    return 0;
}

uint8_t mcp2515_handle_packet(uint8_t type, uint8_t id, uint16_t sensor, const volatile uint8_t *data, uint8_t len)
{
    uint16_t new_periodic_interval;

    if ((type == TYPE_value_explicit || type == TYPE_value_periodic) &&
            sensor == SENSOR_time) {
        uint32_t new_time =
            (uint32_t)data[0] << 24 |
            (uint32_t)data[1] << 16 |
            (uint32_t)data[2] << 8  |
            (uint32_t)data[3] << 0;
        time_set(new_time);
        /* TODO FIXME remove this line VVV*/
        return 1;
    } else if (type == TYPE_set_interval && (id == MY_ID || id == ID_any)) {
        periodic_prev = now;
        new_periodic_interval =
            (uint16_t)data[0] << 8 |
            (uint16_t)data[1] << 0;

        if (new_periodic_interval > 5) {
            periodic_interval = new_periodic_interval;
        }
    } else if (type == TYPE_xfer_cancel && id == MY_ID) {
        puts_P(PSTR("ccl"));
        xfer_state = XFER_CANCEL;
    } else if (type == TYPE_xfer_cts && id == MY_ID) {
        if (xfer_state == XFER_CHUNK_SENT) {
            puts_P(PSTR("cts"));
            xfer_state = XFER_GOT_CTS;
        }
    } else {
        return 1;
    }

    return 0;
}


uint8_t mcp2515_send_sensor(uint8_t type, uint8_t id, uint16_t sensor, const uint8_t *data, uint8_t len)
{
    uint8_t rc;

    mcp2515_handle_packet(type, id, sensor, data, len);

    if (stfu) {
        return 0;
    }

    if (mcp2515_busy) {
        if (busy_since + 5 > uptime) {
            //printf("NO reinit, drop\n");
        } else {
            printf("REINIT REINIT\n");
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                rc = mcp2515_init();
            }
            if (rc != 0) {
                for (;;) {
                    printf("WTF1\n");
                }
            }
        }
        return ERR_MCP_HW;
    }

    load_tx0(type, id, sensor, (const uint8_t *)data, len);
    mcp2515_busy = 1;
    busy_since = uptime;

    modify_register(MCP_REGISTER_CANINTF, MCP_INTERRUPT_TX0I, 0x00);
    mcp2515_select();
    spi_write(MCP_COMMAND_RTS_TX0);
    mcp2515_unselect();

    mcp2515_handle_sos(type, id);

    return 0;
}

void load_tx0(uint8_t type, uint8_t id, uint16_t sensor, const uint8_t *data, uint8_t len)
{
    uint8_t i;

    id = ((id & 0b00011100) << 3) | (1 << EXIDE) | (id & 0b00000011);

    if (len > 8) {
        len = 8;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        mcp2515_select();

        spi_write(MCP_COMMAND_LOAD_TX0);
        spi_write(type);
        spi_write(id);
        spi_write(sensor >> 8);
        spi_write(sensor & 0xFF);

        spi_write(len);

        for (i = 0; i < len; i++) {
            spi_write(data[i]);
        }

        mcp2515_unselect();
    }
}

void read_packet(uint8_t regnum)
{
    uint8_t i;
    uint8_t rc;
    uint8_t type;
    uint8_t id;
    uint16_t sensor;
    uint8_t len;
    uint8_t data[8];

    mcp2515_select();
    spi_write(MCP_COMMAND_READ);
    spi_write(regnum);

    type = spi_recv();
    id = spi_recv();
    id = ((id & 0b11100000) >> 3) | (id & 0b00000011);

    sensor = spi_recv();
    sensor <<= 8;
    sensor |= spi_recv();

    len = spi_recv() & 0x0f;

    if (len > 8) {
        len = 8;
    }

    for (i = 0; i < len; i++) {
        data[i] = spi_recv();
    }

    mcp2515_unselect();

    mcp2515_packet_time = uptime;

    if (MY_ID == ID_any) {
        goto trigger_irq;
    }

    rc = mcp2515_handle_sos(type, id);
    if (rc == 0) {
        printf("SOSd\n");
        return;
    }

    rc = mcp2515_handle_packet(type, id, sensor, data, len);
    if (rc == 0) {
        printf("Handled\n");
        return;
    }

    if (MY_ID == ID_modema || MY_ID == ID_modemb || id == MY_ID) {
        goto trigger_irq;
    }

    return;

trigger_irq:
    if (ring_space(&mcp_ring) < CAN_HEADER_LEN + len) {
        TRIGGER_CAN_IRQ();
        printf("CAN OVERRUN\n");
        return;
    }

    ring_put(&mcp_ring, type);
    ring_put(&mcp_ring, id);
    ring_put(&mcp_ring, (sensor >> 8));
    ring_put(&mcp_ring, (sensor >> 0));
    ring_put(&mcp_ring, len);

    for (i = 0; i < len; i++) {
        ring_put(&mcp_ring, data[i]);
    }

    TRIGGER_CAN_IRQ();
}

ISR(PCINT0_vect)
{
    uint8_t canintf;

    canintf = read_register(MCP_REGISTER_CANINTF);

    if (canintf & MCP_INTERRUPT_ERRI) {
        printf("MCP: ERRI\n");
        uint8_t rc;
        rc = mcp2515_init();
        printf("DONE REINIT\n");
        if (rc != 0) {
            for (;;) {
                printf("WTF2\n");
            }
        }
        return;
    }

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

    /* we IGNORE merr */
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
