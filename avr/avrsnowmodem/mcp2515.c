#include <avr/io.h>
#include <util/atomic.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>

#include "mcp2515.h"
#include "spi.h"

struct mcp2515_packet_t {
    uint32_t id;
    uint8_t priority;
    uint8_t length;
    uint8_t data[8];
};

#define MCP2515_PORT PORTB
#define MCP2515_CS PORTB2
#define MCP2515_DDR DDRB

#define RXBUFSZ 4
#define TXBUFSZ 6
volatile struct mcp2515_packet_t rx_buffer[RXBUFSZ];
volatile struct mcp2515_packet_t tx_buffer[TXBUFSZ];
volatile uint8_t rx_buf_in;
volatile uint8_t rx_buf_out;
volatile uint8_t tx_buf_in;
volatile uint8_t tx_buf_out;

#define BUF_PUSH(buf,in,out,size,val) do { \
    buf[in] = val; \
    in++; \
    in %= size; } while(0)
    

static uint8_t buf_empty(uint8_t in, uint8_t out)
{
    return in == out;
}

static uint8_t bytes_in_ring(uint8_t buf_in, uint8_t buf_out, uint8_t buf_size)
{
    if (buf_in > buf_out) {
        return (buf_in - buf_out);
    } else if (buf_out > buf_in) {
        return (buf_size - (buf_out - buf_in));
    } else {
        return 0;  		
    }
}

static uint8_t buf_full(uint8_t in, uint8_t out, uint8_t size)
{
    return bytes_in_ring(in, out, size) == size;
}


inline void device_select(void)
{
    MCP2515_PORT &= ~(1 << MCP2515_CS);
}

inline void device_unselect(void)
{
    MCP2515_PORT |= (1 << MCP2515_CS);
}

uint8_t read_register_single(uint8_t address)
{
    uint8_t result;

    device_select();

    spi_write(MCP_COMMAND_READ);
    spi_write(address);
    result = spi_recv();

    device_unselect();

    return result;
}

static void read_register(uint8_t address, uint8_t *values, uint8_t count)
{
    device_select();

    spi_write(MCP_COMMAND_READ);
    spi_write(address);

    while (count > 0) {
        *values++ = spi_recv();
        --count;
    }

    device_unselect();
}

void write_register_single(uint8_t address, uint8_t value)
{
    device_select();
    
    spi_write(MCP_COMMAND_WRITE);
    spi_write(address);
    spi_write(value);

    device_unselect();
}

static void write_register(uint8_t address, const uint8_t *values, uint8_t count)
{
    device_select();

    spi_write(MCP_COMMAND_WRITE);
    spi_write(address);

    while (count > 0) {
        spi_write(*values++);
        --count;
    }

    device_unselect();
}

void modify_register(uint8_t address, uint8_t mask, const uint8_t value)
{
    device_select();
    
    spi_write(MCP_COMMAND_BITMOD);
    spi_write(address);
    spi_write(mask);
    spi_write(value);

    device_unselect();
}


// ----------------------------------------------------------------------------------------
//  mapping the four register bytes to the uint32_t CAN identifier: 
//
//  + the first eleven bits contain the standard ID
//  + the next eighteen bits contain the extended ID
//
//      00000000000000000000000000000000  uint32_t
//
//      3   2   2   1   1   1   7   3  0
//      1   7   3   9   5   1
//
//                           +--+------+  std
//         +-+------++------+             ext
//
//                           76543210     SIDH
//         10                        765  SIDL
//           76543210                     EID8
//                   76543210             EID0

static uint32_t get_can_id(uint8_t sidh_register)
{
    uint8_t address_data[4];  // SIDH, SIDL, EID8, EID0

    read_register(sidh_register, address_data, 4);

    uint32_t result;
     
    result  = (uint32_t)address_data[0] << 3;
    result |= (uint32_t)address_data[1] >> 5;
    result |= ((uint32_t)address_data[1] & 0x3) << 27;
    result |= (uint32_t)address_data[3] << 11;
    result |= (uint32_t)address_data[2] << 19;

    return result;
}

static void set_can_id( uint8_t sidh_register, uint32_t id )
{
    uint8_t address_data[4];  // SIDH, SIDL, EID8, EID0

    address_data[0] = (uint8_t)(id >> 3);
    address_data[1] = (uint8_t)id << 5;
    id >>= 11;
    address_data[3] = (uint8_t)id;
    id >>= 8;
    address_data[2] = (uint8_t)id;
    id >>= 8;
    address_data[1] |= (uint8_t)id & 0x3;
    address_data[1] |= (1 << 3); // EXIDE

    write_register(sidh_register, address_data, 4);
}


static void read_into_rx_buffer(uint8_t rx_ctrl_register)    
{
    struct mcp2515_packet_t msg;

    //--    Copy out the message identifier.

    msg.id = get_can_id(rx_ctrl_register + 1 /* SIDH */);

    //--    Retrieve the request flag or data.

    uint8_t dlc = read_register_single(rx_ctrl_register + 5 /* DLC */);

    if (dlc & (1 << 6 )){ // RTR bit
        msg.length = 0xff;  // signifies this is a request rather than a message
    } else {
        msg.length = dlc;
        read_register(rx_ctrl_register + 6, msg.data, dlc);
    }
    
    //--    Add it to the queue.
    BUF_PUSH(rx_buffer, rx_buf_in, rx_buf_out, RXBUFSZ, msg);
}


static void send_from_tx_buffer(void)
{
    struct mcp2515_packet_t msg;
    
    msg = tx_buffer[tx_buf_out];
    tx_buf_out++;
    tx_buf_out %= TXBUFSZ;

    //--    Load the destination address.

    set_can_id(MCP_REGISTER_TXB0CTRL + 1 /* SIDH */, msg.id);

    //--    Request or message? Set up the registers appropriately.

    if (msg.length == 0xff) {
        write_register_single(MCP_REGISTER_TXB0CTRL + 5 /* DLC */, (1 << 6));
    } else {
        //--    Load the message contents.

        write_register_single(MCP_REGISTER_TXB0CTRL + 5 /* DLC */, msg.length);

        write_register(MCP_REGISTER_TXB0CTRL + 6 /* DATA */,
                       msg.data, 
                       msg.length);
    }

    //--    Fire off the transmit.

    modify_register(MCP_REGISTER_TXB0CTRL, 0x0b, (0x08 | msg.priority));
}

uint8_t mcp2515_init(void)
{
    spi_init();

    MCP2515_DDR |= (1 << MCP2515_CS);
    device_unselect();

    mcp2515_reset();

    //--    Set mode to configuration operation.
    modify_register( MCP_REGISTER_CANCTRL, 0xe0, 0x80 );
    
    if ((read_register_single(MCP_REGISTER_CANSTAT) & 0xe0) != 0x80) {
        return 0;
    }

    //--    Set up the bus speed via the bit timing registers; this depends on the input
    //      clock frequency for the chip.

//#if defined( PRJ_MCP2515_CANBUS_20_KHZ ) && defined( PRJ_MCP2515_FREQUENCY_4_MHZ )

    //--    SWJ = 0 (1 x TQ), BRP = 4
    write_register_single( MCP_REGISTER_CNF1, ( ( 0 << 6 ) | ( 4 ) ) );

    //--    BLTMODE = 1, SAM = 0, PHSEG1 = 7 (8 x TQ), PRSEG = 2 (3 x TQ)
    write_register_single( MCP_REGISTER_CNF2, ( ( 1 << 7 ) | ( 0 << 6 ) | ( 7 << 3 ) | ( 2 ) ) );
        
    //--    SOF = 0, WAKFIL = 0, PHSEG2 = 7 (8 x TQ)
    write_register_single( MCP_REGISTER_CNF3, ( ( 0 << 7 ) | ( 0 << 6 ) | ( 7 ) ) );

    //--    Reset the three transmit buffers.

    /*
    write_register_single( MCP_REGISTER_TXB0CTRL, 0x00 );
    write_register_single( MCP_REGISTER_TXB1CTRL, 0x00 );
    write_register_single( MCP_REGISTER_TXB2CTRL, 0x00 );

    //--    Reset the two receive buffers.

    write_register_single( MCP_REGISTER_RXB0CTRL, 0x00 );
    write_register_single( MCP_REGISTER_RXB1CTRL, 0x00 );

    //--    Clear all receive filters.

    set_can_id( MCP_REGISTER_RXF0SIDH, 0x0000 );
    set_can_id( MCP_REGISTER_RXF1SIDH, 0x0000 );
    set_can_id( MCP_REGISTER_RXF2SIDH, 0x0000 );
    set_can_id( MCP_REGISTER_RXF3SIDH, 0x0000 );
    set_can_id( MCP_REGISTER_RXF4SIDH, 0x0000 );
    set_can_id( MCP_REGISTER_RXF5SIDH, 0x0000 );

    //--    Enable both receive-buffers to receive messages with standard and extended ids;
    //      enable message receive rollover.

    write_register_single( MCP_REGISTER_RXB0CTRL,
                    ( ( 1 << 6 )          // RMX1
                    | ( 1 << 5 )          // RMX0
                    | ( 0 << 3 )          // RXRTR
                    | ( 1 << 2 )          // BUKT
                    | ( 0 << 1 )          // BUKT1
                    | ( 0 << 0 ) ) );     // FILHIT0

    write_register_single( MCP_REGISTER_RXB1CTRL,
                    ( ( 1 << 6 )          // RMX1
                    | ( 1 << 5 )          // RMX0
                    | ( 0 << 3 )          // RXRTR
                    | ( 0 << 2 )          // BUKT
                    | ( 0 << 1 )          // BUKT1
                    | ( 0 << 0 ) ) );     // FILHIT0

    //--    Set up the interrupt-driven RX and TX, if required.

    */
    //modify_register( MCP_REGISTER_CANINTF, MCP_INTERRUPT_TX0I, 0xff );
    modify_register( MCP_REGISTER_CANINTE, MCP_INTERRUPT_RX0I | MCP_INTERRUPT_RX1I, 0xff );

    //printf("caninte: %x\n", read_register_single(MCP_REGISTER_CANINTE));

    /* filters off */
    write_register_single(MCP_REGISTER_RXB0CTRL, 0b01100000);

    //--    Set mode to loopback operation.
    modify_register( MCP_REGISTER_CANCTRL, 0xff, 0b00001000);
    modify_register( MCP_REGISTER_CANCTRL, 0xe0, 0b00000000);

    uint8_t retry = 10;
    while (( read_register_single( MCP_REGISTER_CANSTAT ) & 0xe0 ) != 0b00000000 && --retry){
    }
    if (retry == 0) {
        //printf("canstat not correct: %x\n", read_register_single( MCP_REGISTER_CANSTAT ));
        return 0;
    }

    return 1;
}

void mcp2515_reset(void)
{
    //--    Wait for the hardware to stabilize; this isn't required on resets other than
    //      the first, but it doesn't hurt.

    _delay_ms(10);

    device_select();
    spi_write(MCP_COMMAND_RESET);
    device_unselect();
    _delay_ms(10);    // approximately long enough
}

void mcp2515_process_interrupt()
{
    uint8_t canintf = read_register_single( MCP_REGISTER_CANINTF );

    if ( canintf & MCP_INTERRUPT_RX1I ) {
        read_into_rx_buffer( MCP_REGISTER_RXB1CTRL );
        modify_register( MCP_REGISTER_CANINTF, MCP_INTERRUPT_RX1I, 0x00 );
    }

    if ( canintf & MCP_INTERRUPT_RX0I ) {
        read_into_rx_buffer( MCP_REGISTER_RXB0CTRL );
        modify_register( MCP_REGISTER_CANINTF, MCP_INTERRUPT_RX0I, 0x00 );
    }
    
    if ( canintf & MCP_INTERRUPT_TX0I ) {
        if (buf_empty(tx_buf_in, tx_buf_out)) {
            modify_register( MCP_REGISTER_CANINTE, MCP_INTERRUPT_TX0I, 0x00 );
        } else {
            modify_register( MCP_REGISTER_CANINTF, MCP_INTERRUPT_TX0I, 0x00 );
            send_from_tx_buffer();
        }
    }
}

uint8_t mcp2515_read    
    (
        uint32_t* can_id,
        uint8_t*     is_request,
        uint8_t*  data_ptr,
        uint8_t*  length
    )
{
    uint8_t i;
    
    //--    Pending message?
    if (buf_empty(rx_buf_in, rx_buf_out)) {
        return 0;
    }

    //--    Fetch it from the queue and copy it out.

    struct mcp2515_packet_t msg = rx_buffer[rx_buf_out];
    rx_buf_out++;
    rx_buf_out %= RXBUFSZ;

    *can_id = msg.id;
    
    if (msg.length == 0xff) {
        *is_request = 1;
    } else {
        *is_request = 0;
        *length = msg.length;

        for (i = 0; i < msg.length; i++) {
            *data_ptr++ = msg.data[i];
        }
    }

    return 1;
}

uint8_t mcp2515_send
    (
        const uint32_t can_id,
        const uint8_t*  data_ptr,
        uint8_t         length,
        uint8_t         priority
    )
{
    uint8_t i;
    
    //--    Got space in the buffer?

    if (buf_full(tx_buf_in, tx_buf_out, TXBUFSZ)) {
        return 0;
    }

    //--    Add the message to the buffer, then enable the interrupt to trigger a
    //      transmit. Note this must be done with interrupts disabled to avoid a
    //      race condition that hangs here.

    struct mcp2515_packet_t msg;
    
    msg.id = can_id;
    msg.priority = priority;
    msg.length = length;

    for (i = 0; i < length; i++) {
        msg.data[i] = *data_ptr++;
    }
    
    BUF_PUSH(tx_buffer, tx_buf_in, tx_buf_out, TXBUFSZ, msg);

    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        modify_register(MCP_REGISTER_CANINTE, MCP_INTERRUPT_TX0I, 0xff);
    }

    return 1;
}


// ----------------------------------------------------------------------------------------
    
uint8_t mcp2515_request(const uint32_t can_id, uint8_t priority)
{
    //--    Got space in the buffer?
    if (buf_full(tx_buf_in, tx_buf_out, TXBUFSZ)) {
        return 0;
    }

    //--    Add the message to the buffer, then enable the interrupt to trigger a
    //      transmit. Note this must be done with interrupts disabled to avoid a
    //      race condition that hangs here.

    struct mcp2515_packet_t msg;
    
    msg.id = can_id;
    msg.priority = priority;
    msg.length = 0xff; // request, not an actual message
    
    BUF_PUSH(tx_buffer, tx_buf_in, tx_buf_out, TXBUFSZ, msg);

    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        modify_register( MCP_REGISTER_CANINTE, MCP_INTERRUPT_TX0I, 0xff );
    }

    return 1;
}

static void print_register(const char* PROGMEM label, uint8_t address)
{
    uint8_t i;
    uint8_t value = read_register_single(address);

    printf_P(label);
    
    for (i = 0; i < 8; i++) {
        if (value & 0x80) {
            putchar('1');
        } else {
            putchar('0');
        }
        value = value << 1;
    }

    putchar('\n');
}

void mcp2515_debug_print_registers()
{
    printf_P( PSTR("MCP2515 registers:\n") );

    print_register( PSTR("    TXRTSCTRL: "), MCP_REGISTER_TXRTSCTRL );
    print_register( PSTR("    CANSTAT  : "), MCP_REGISTER_CANSTAT );
    print_register( PSTR("    CANCTRL  : "), MCP_REGISTER_CANCTRL );
    putchar( '\n' );

    print_register( PSTR("    TXB0CTRL : "), MCP_REGISTER_TXB0CTRL );
    print_register( PSTR("    TXB0SIDH : "), MCP_REGISTER_TXB0SIDH );
    print_register( PSTR("    TXB0SIDL : "), MCP_REGISTER_TXB0SIDL );
    print_register( PSTR("    TXB0EID8 : "), MCP_REGISTER_TXB0EID8 );
    print_register( PSTR("    TXB0EID0 : "), MCP_REGISTER_TXB0EID0 );
    printf_P( PSTR("    TXB0DLC  : %u"), read_register_single( MCP_REGISTER_TXB0DLC ) );
    putchar( '\n' );
    
    print_register( PSTR("    TXB1CTRL : "), MCP_REGISTER_TXB1CTRL );
    print_register( PSTR("    TXB1SIDH : "), MCP_REGISTER_TXB1SIDH );
    print_register( PSTR("    TXB1SIDL : "), MCP_REGISTER_TXB1SIDL );
    print_register( PSTR("    TXB1EID8 : "), MCP_REGISTER_TXB1EID8 );
    print_register( PSTR("    TXB1EID0 : "), MCP_REGISTER_TXB1EID0 );
    printf_P( PSTR("    TXB1DLC  : %u"), read_register_single( MCP_REGISTER_TXB1DLC ) );
    putchar( '\n' );
    
    print_register( PSTR("    TXB2CTRL : "), MCP_REGISTER_TXB2CTRL );
    print_register( PSTR("    TXB2SIDH : "), MCP_REGISTER_TXB2SIDH );
    print_register( PSTR("    TXB2SIDL : "), MCP_REGISTER_TXB2SIDL );
    print_register( PSTR("    TXB2EID8 : "), MCP_REGISTER_TXB2EID8 );
    print_register( PSTR("    TXB2EID0 : "), MCP_REGISTER_TXB2EID0 );
    printf_P( PSTR("    TXB2DLC  : %u"), read_register_single( MCP_REGISTER_TXB2DLC ) );
    putchar( '\n' );
    
    print_register( PSTR("    RXB0CTRL : "), MCP_REGISTER_RXB0CTRL );
    print_register( PSTR("    RXB0SIDH : "), MCP_REGISTER_RXB0SIDH );
    print_register( PSTR("    RXB0SIDL : "), MCP_REGISTER_RXB0SIDL );
    print_register( PSTR("    RXB0EID8 : "), MCP_REGISTER_RXB0EID8 );
    print_register( PSTR("    RXB0EID0 : "), MCP_REGISTER_RXB0EID0 );
    printf_P( PSTR("    RXB0DLC  : %u"), read_register_single( MCP_REGISTER_RXB0DLC ) );
    putchar( '\n' );
    
    print_register( PSTR("    RXB1CTRL : "), MCP_REGISTER_RXB1CTRL );
    print_register( PSTR("    RXB1SIDH : "), MCP_REGISTER_RXB1SIDH );
    print_register( PSTR("    RXB1SIDL : "), MCP_REGISTER_RXB1SIDL );
    print_register( PSTR("    RXB1EID8 : "), MCP_REGISTER_RXB1EID8 );
    print_register( PSTR("    RXB1EID0 : "), MCP_REGISTER_RXB1EID0 );
    printf_P( PSTR("    RXB0DLC  : %u"), read_register_single( MCP_REGISTER_RXB1DLC ) );
    putchar( '\n' );
    
    for ( uint8_t i = 0; i < 0x80; i++ )
    {
        if ( ( ( i % 0x10 ) == 0 ) && ( i != 0 ) )
        {
            putchar( '\n' );
            printf_P( PSTR("0x%X: "), i );
        }
        else if ( i == 0 )
        {
            printf_P( PSTR("0x00: ") );
        }
    
        uint8_t reg = read_register_single( i );
    
        printf_P( PSTR("0x") );
        if ( reg < 0x10 )
        {
            putchar( '0' );
        }
        printf_P( PSTR("%X "), reg );
    }

    putchar( '\n' );
}

void read_ff_0(uint8_t *length, uint8_t *buf, uint16_t *frame_id)
{
    uint8_t len, i;
    uint16_t id_h, id_l;

    device_select();
    spi_write(0x90);
    id_h = spi_recv();
    id_l = spi_recv();
    spi_recv(); // extended id (unused)
    spi_recv(); // extended id (unused)

    len = spi_recv() & 0x0f;
    for(i=0;i<len;i++){
        buf[i] = spi_recv();
    }

    *length = len;

    *frame_id = ((((uint16_t) id_h) << 3) + ((id_l & 0xe0) >> 5));
}




void load_ff_0(uint8_t length, uint16_t identifier, uint8_t *data)
{
    uint8_t i,id_high,id_low;

    //generate id bytes before SPI write
    id_high = (uint8_t)(identifier >> 3);
    id_low = (uint8_t)(identifier << 5);

    device_select();
    spi_write(0x40);
    spi_write(id_high); //identifier high bits
    spi_write(id_low); //identifier low bits
    spi_write(0x00); //extended identifier registers(unused)
    spi_write(0x00);
    spi_write(length);
    for (i=0;i<length;i++) { //load data buffer
            spi_write(data[i]);
    }

    device_unselect();
}
