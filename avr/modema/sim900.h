enum state {
    STATE_UNKNOWN,
    STATE_CLOSED,
    STATE_IP_INITIAL,
    STATE_IP_START,
    STATE_IP_GPRSACT,
    STATE_IP_STATUS,
    STATE_IP_PROCESSING,
    STATE_CONNECTED,
    STATE_EXPECTING_PROMPT,
    STATE_GOT_PROMPT,
    STATE_POWEROFF,
    STATE_ERROR,
};

extern volatile enum state state;
extern volatile uint8_t flag_ok;
extern volatile uint8_t flag_error;
extern int modemFD;

void sendATCommand(const char *fmt, ...);
uint8_t TCPSend(uint8_t *buf, uint16_t count);
uint8_t TCPConnect(void);
uint8_t TCPDisconnect(void);

void slow_write(const char *buf, uint16_t count);
