#define TYPE_LIST(X) \
    X(value_periodic, 0) \
    X(value_explicit, 1) \
    X(set_time, 2) \
    X(set_interval, 3) \
    X(sos_reboot, 4) \
    X(sos_rx_overrun, 5) \
    X(sos_stfu, 6) \
    X(sos_nostfu, 7) \
\
    X(file_chdir, 0x11) \
\
    X(file_error, 0x12) \
\
    X(sensor_error, 0x13) \
\
\
    X(file_checksum, 0x15) \
\
\
    X(get_arm, 0x20) \
    X(set_arm, 0x21) \
\
    X(get_output, 0x22) \
    X(set_output, 0x23) \
\
    X(get_value, 0x24) \
    X(set_value, 0x25) \
\
    X(xfer_cts, 0x70) \
    X(xfer_cancel, 0x71) \
    X(xfer_data, 0x72) \
    X(full_log_offer, 0x73) \
    X(request_list_files, 0x74) \
    X(file_listing, 0x75) \
    X(request_file, 0x76) \
    X(file_contents, 0x77) \
    X(file_write, 0x78) \
\
    X(invalid, 0xff) \


#define ID_LIST(X) \
    X(any,     0x00) \
    X(ping,    0x01) \
    X(pong,    0x02) \
    X(laser,   0x03) \
    X(gps,     0x04) \
    X(temp,    0x05) \
    X(time,    0x06) \
    X(sd,      0x07) \
    X(arm,     0x08) \
    X(heater,  0x09) \
    X(lidar,   0x0a) \
    X(powershot, 0x0b) \
    X(9dof,    0x0c) \
    X(compass, 0x0d) \
	 X(modema,  0x0e) \
  	 X(modemb,  0x0f) \
	 X(accel,   0x10) \
	 X(gyro,    0x11) \
    X(invalid, 0x1f) \


#define TYPE_XFER(type) ((type & 0xf0) == 0x70)

enum type {
#define X(name, value) TYPE_ ## name = value, \

    TYPE_LIST(X)
#undef X
};


enum id {
#define X(name, value) ID_ ## name = value, \

    ID_LIST(X)
#undef X
};

struct mcp2515_packet_t {
    uint8_t type;
    uint8_t id;
    uint8_t more;
    uint8_t len;
    uint8_t data[9];
    uint8_t unread;
};

enum {
    IRQ_CAN   = (1 << 0),
    IRQ_TIMER = (1 << 1),
    IRQ_UART  = (1 << 2),
};

extern volatile uint8_t mcp2515_busy;
extern volatile uint8_t irq_signal;
extern volatile struct mcp2515_packet_t packet;
void mcp2515_irq(void);
typedef uint8_t (*mcp2515_xfer_callback_t)(void);

#define BRP0        0
#define BTLMODE     7
#define SAM         6
#define PHSEG12     5
#define PHSEG11     4
#define PHSEG10     3
#define PHSEG2      2
#define PHSEG1      1
#define PHSEG0      0

#define SJW1        7
#define SJW0        6
#define BRP5        5
#define BRP4        4
#define BRP3        3
#define BRP2        2
#define BRP1        1
#define BRP0        0
#define WAKFIL      6
#define PHSEG22     2
#define PHSEG21     1
#define PHSEG20     0


#define RXM1 6
#define RXM0 5
#define BUKT 1

#define EXIDE 3

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
#define MCP_REGISTER_EFLG     0x2D

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

uint8_t mcp2515_init(void);
void mcp2515_reset(void);
//void read_ff_0(uint8_t *length, uint8_t *buf, uint16_t *frame_id);
uint8_t read_register(uint8_t address);
void write_register(uint8_t address, uint8_t value);
void modify_register(uint8_t address, uint8_t mask, const uint8_t value);
uint8_t mcp2515_send(uint8_t type, uint8_t id, uint8_t len, const void *data);
uint8_t mcp2515_send2(struct mcp2515_packet_t *p);
void load_tx0(uint8_t type, uint8_t id, uint8_t len, const uint8_t *data);

uint8_t mcp2515_xfer(uint8_t type, uint8_t dest, uint8_t len, void *data);
uint8_t mcp2515_receive_xfer_wait(uint8_t type, uint8_t sender_id,
    mcp2515_xfer_callback_t xfer_cb);
uint8_t mcp2515_send_xfer_wait(struct mcp2515_packet_t *p);
