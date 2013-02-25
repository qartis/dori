#define MCP_COMMAND_WRITE   0x02
#define MCP_COMMAND_READ    0x03
#define MCP_COMMAND_BITMOD  0x05

#define MCP_COMMAND_LOAD_TX0  0x40
#define MCP_COMMAND_LOAD_TX1  0x42
#define MCP_COMMAND_LOAD_TX2  0x44

#define MCP_COMMAND_RTS_TX0  0x81
#define MCP_COMMAND_RTS_TX1  0x82
#define MCP_COMMAND_RTS_TX2  0x84
#define MCP_COMMAND_RTS_ALL  0x87

#define MCP_COMMAND_READ_RX0  0x90
#define MCP_COMMAND_READ_RX1  0x94

#define MCP_COMMAND_READ_STATUS  0xa0
#define MCP_COMMAND_RX_STATUS    0xb0

#define MCP_COMMAND_RESET  0xc0

#define MCP_MODE_NORMAL      0x00
#define MCP_MODE_SLEEP       0x20
#define MCP_MODE_LOOPBACK    0x40
#define MCP_MODE_LISTENONLY  0x60
#define MCP_MODE_CONFIG      0x80
#define MCP_MODE_POWERUP     0xE0

#define MCP_INTERRUPT_MERR  (1 << 7)
#define MCP_INTERRUPT_WAKI  (1 << 6)
#define MCP_INTERRUPT_ERRI  (1 << 5)
#define MCP_INTERRUPT_TX2I  (1 << 4)
#define MCP_INTERRUPT_TX1I  (1 << 3)
#define MCP_INTERRUPT_TX0I  (1 << 2)
#define MCP_INTERRUPT_RX1I  (1 << 1)
#define MCP_INTERRUPT_RX0I  (1 << 0)

#define MCP_REGISTER_RXF0SIDH  0x00
#define MCP_REGISTER_RXF0SIDL  0x01
#define MCP_REGISTER_RXF0EID8  0x02
#define MCP_REGISTER_RXF0EID0  0x03

#define MCP_REGISTER_RXF1SIDH  0x04
#define MCP_REGISTER_RXF1SIDL  0x05
#define MCP_REGISTER_RXF1EID8  0x06
#define MCP_REGISTER_RXF1EID0  0x07

#define MCP_REGISTER_RXF2SIDH  0x08
#define MCP_REGISTER_RXF2SIDL  0x09
#define MCP_REGISTER_RXF2EID8  0x0a
#define MCP_REGISTER_RXF2EID0  0x0b

#define MCP_REGISTER_RXF3SIDH  0x10
#define MCP_REGISTER_RXF3SIDL  0x11
#define MCP_REGISTER_RXF3EID8  0x12
#define MCP_REGISTER_RXF3EID0  0x13

#define MCP_REGISTER_RXF4SIDH  0x14
#define MCP_REGISTER_RXF4SIDL  0x15
#define MCP_REGISTER_RXF4EID8  0x16
#define MCP_REGISTER_RXF4EID0  0x17

#define MCP_REGISTER_RXF5SIDH  0x18
#define MCP_REGISTER_RXF5SIDL  0x19
#define MCP_REGISTER_RXF5EID8  0x1a
#define MCP_REGISTER_RXF5EID0  0x1b

#define MCP_REGISTER_TEC  0x1c
#define MCP_REGISTER_REC  0x1d

#define MCP_REGISTER_CNF1  0x2a
#define MCP_REGISTER_CNF2  0x29
#define MCP_REGISTER_CNF3  0x28

#define MCP_REGISTER_CANINTE  0x2B
#define MCP_REGISTER_CANINTF  0x2C

#define MCP_REGISTER_TXB0CTRL  0x30
#define MCP_REGISTER_TXB0SIDH  0x31
#define MCP_REGISTER_TXB0SIDL  0x32
#define MCP_REGISTER_TXB0EID8  0x33
#define MCP_REGISTER_TXB0EID0  0x34
#define MCP_REGISTER_TXB0DLC   0x35
#define MCP_REGISTER_TXB0DATA  0x36

#define MCP_REGISTER_TXB1CTRL  0x40
#define MCP_REGISTER_TXB1SIDH  0x41
#define MCP_REGISTER_TXB1SIDL  0x42
#define MCP_REGISTER_TXB1EID8  0x43
#define MCP_REGISTER_TXB1EID0  0x44
#define MCP_REGISTER_TXB1DLC   0x45
#define MCP_REGISTER_TXB1DATA  0x46

#define MCP_REGISTER_TXB2CTRL  0x50
#define MCP_REGISTER_TXB2SIDH  0x51
#define MCP_REGISTER_TXB2SIDL  0x52
#define MCP_REGISTER_TXB2EID8  0x53
#define MCP_REGISTER_TXB2EID0  0x54
#define MCP_REGISTER_TXB2DLC   0x55
#define MCP_REGISTER_TXB2DATA  0x56

#define MCP_REGISTER_RXB0CTRL  0x60
#define MCP_REGISTER_RXB0SIDH  0x61
#define MCP_REGISTER_RXB0SIDL  0x62
#define MCP_REGISTER_RXB0EID8  0x63
#define MCP_REGISTER_RXB0EID0  0x64
#define MCP_REGISTER_RXB0DLC   0x65
#define MCP_REGISTER_RXB0DATA  0x66

#define MCP_REGISTER_RXB1CTRL  0x70
#define MCP_REGISTER_RXB1SIDH  0x71
#define MCP_REGISTER_RXB1SIDL  0x72
#define MCP_REGISTER_RXB1EID8  0x73
#define MCP_REGISTER_RXB1EID0  0x74
#define MCP_REGISTER_RXB1DLC   0x75
#define MCP_REGISTER_RXB1DATA  0x76

#define MCP_REGISTER_TXRTSCTRL  0x0d
#define MCP_REGISTER_CANSTAT    0x0e
#define MCP_REGISTER_CANCTRL    0x0f


#define mcp2515_max_message_data  0x08

#define mcp2515_priority_urgent  0x03
#define mcp2515_priority_high    0x02
#define mcp2515_priority_normal  0x01
#define mcp2515_priority_low     0x00

//      Initialize the MCP2515 h/w according to the project_defs.h specification.
//      Returns true if the initialization was successful.
uint8_t mcp2515_init(void);
    
//      Reset the MCP2515 h/w. Any pending messages in the transmit or recieve buffers
//      are lost.
void mcp2515_reset(void);


//      Respond to the INTERRUPT pin transitioning to a low state.
//      Only compiled if PRJ_MCP2515_USE_INTERRUPT_RXTX is defined.
void mcp2515_process_interrupt(void);


// ----------------------------------------------------------------------------------------
//  mcp2515_read
//
//      Read a pending message from one of the MCP2515 receive buffers. Does not block if
//      no message is available.
//
//      Returns true if a message was successfully received.
//

uint8_t mcp2515_read    
    (
        uint32_t* can_id,
        uint8_t* is_request,
        uint8_t* data_ptr,
        uint8_t* length
    );

uint8_t mcp2515_send
    (
        const uint32_t can_id,
        const uint8_t* data_ptr,
        uint8_t length,
        uint8_t priority 
    );

uint8_t mcp2515_request(const uint32_t can_id, uint8_t priority);
void mcp2515_debug_print_registers(void);
void load_ff_0(uint8_t length, uint16_t identifier, uint8_t *data);
void read_ff_0(uint8_t *length, uint8_t *buf, uint16_t *frame_id);
uint8_t read_register_single( uint8_t address );
void write_register_single(uint8_t address, uint8_t value);
void modify_register(uint8_t address, uint8_t mask, const uint8_t value);
