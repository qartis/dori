enum state {
    STATE_CLOSED = 1,
    STATE_IP_INITIAL = 2,
    STATE_IP_START = 3,
    STATE_IP_GPRSACT = 4,
    STATE_IP_STATUS = 5,
    STATE_IP_PROCESSING = 6,
    STATE_CONNECTED = 7,
    STATE_EXPECTING_PROMPT = 8,
    STATE_GOT_PROMPT = 9,
    STATE_POWEROFF = 10,
    STATE_ERROR = 11,
};

extern volatile uint8_t can_ring_array[128];
extern volatile struct ring_t can_ring;
extern uint8_t can_ring_urgent;

extern volatile enum state state;
extern volatile uint8_t flag_ok;
extern volatile uint8_t flag_error;
extern int modem_fd;
extern uint8_t ip[4];

void send_at_cmd(const char *fmt, ...);
uint8_t tcp_send(void);
uint8_t tcp_connect(void);
uint8_t tcp_disconnect(void);

void slow_write(const void *buf, uint16_t count);
