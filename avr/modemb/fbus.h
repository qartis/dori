uint8_t fbus_sendsms(const char *num, const char *msg);
uint8_t fbus_heartbeat(void);
uint8_t fbus_subscribe(void);
uint8_t fbus_delete_sms(uint16_t loc);
void fbus_init(void);

void unbcd_phonenum(volatile char *dest, volatile uint8_t *data, uint8_t len);
void sendack(uint8_t type, uint8_t seqnum);
uint8_t unpack7_msg(volatile uint8_t *data, uint8_t len, volatile char* output);

#define MSG_BUF_SIZE 140

extern volatile char phonenum_buf[16];
extern volatile char msg_buf[MSG_BUF_SIZE];
extern volatile uint8_t msg_buflen;

extern volatile enum fbus_frametype frametype;

extern volatile uint8_t fbusseqno;
extern volatile uint8_t fbustype;

extern volatile uint8_t fbus_ack_flag;
extern volatile uint8_t fbus_sms_sent_flag;
extern volatile uint8_t fbus_id_flag;

#define TYPE_SMS_MGMT 0x14
#define TYPE_SMS 0x02
#define TYPE_ACK 0x7f
#define TYPE_ID  0x1b
#define TYPE_NET_STATUS 0x0a

enum fbus_frametype {
    FRAME_NONE = 0,
    FRAME_UNKNOWN,
    FRAME_ID,
    FRAME_SUBSMS_INCOMING,
    FRAME_SUBSMS_SEND_STATUS,
    FRAME_NET_STATUS,
};
