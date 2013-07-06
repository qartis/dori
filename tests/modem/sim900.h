enum state {
    STATE_UNKNOWN,
    STATE_CLOSED,
    STATE_IP_INITIAL,
    STATE_IP_START,
    STATE_IP_GPRSACT,
    STATE_IP_STATUS,
    STATE_CONNECTED,
    STATE_EXPECTING_PROMPT,
    STATE_GOT_PROMPT,
    STATE_ERROR,
};

volatile enum state state;
extern int modemFD;

void sendATCommand(const char *fmt, ...);
uint8_t TCPSend(void);
uint8_t TCPConnect(void);
uint8_t TCPDisconnect(void);

int modem_init(const char *device);

size_t slow_write(int fd, const char *buf, size_t count);
void _delay_ms(int delay);
