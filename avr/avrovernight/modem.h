void slow_write(const char *buf);

void find_modem(void);
uint8_t connect(void);
uint8_t disconnect(void);

uint8_t send_packet(uint8_t type, uint8_t *buf, uint32_t len);

extern volatile uint8_t packet_buf[512];
extern volatile uint16_t packet_len;
extern volatile uint8_t packet_type;

enum {
    TYPE_PING = 0xff,
    TYPE_TEMP = 0x02,
    TYPE_NUNCHUCK = 0x03,
};

