enum fbus_frametype fbus_readframe(uint8_t timeout);
uint8_t fbus_sendsms(const char *num, const char *msg);
enum fbus_frametype fbus_heartbeat(void);
void fbus_delete_sms(uint8_t memory_type, uint8_t storage_loc);

extern char phonenum_buf[16];
extern char msg_buf[32];

enum fbus_frametype {
    FRAME_READ_TIMEOUT,
    FRAME_SMS_SENT,
    FRAME_SMS_RECV,
    FRAME_SMS_ERROR,
    FRAME_ACK,
    FRAME_ID,
    FRAME_NET_STATUS,
    FRAME_UNKNOWN,
};
