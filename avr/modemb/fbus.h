void fbus_sendsms(const char *num, const char *msg);
enum fbus_frametype fbus_heartbeat(void);
void fbus_subscribe(void);
void fbus_delete_sms(uint8_t memory_type, uint8_t storage_loc);
void fbus_init(void);

void unbcd_phonenum(volatile uint8_t *data);
void sendack(uint8_t type, uint8_t seqnum);
uint8_t unpack7_msg(volatile uint8_t *data, uint8_t len, volatile char* output);

#define MSG_BUF_SIZE 140

extern volatile char phonenum_buf[16];
extern volatile char msg_buf[MSG_BUF_SIZE];
extern volatile uint8_t msg_buflen;

extern volatile enum fbus_frametype frametype;

#define TYPE_SMS_MGMT 0x14
#define TYPE_SMS 0x02
#define TYPE_ACK 0x7f
//#define TYPE_GETID 0xd1
#define TYPE_GETID 0x1b
#define TYPE_ID  0x1b
#define TYPE_NET_STATUS 0x0a

enum fbus_frametype {
    FRAME_NONE = 0,
    FRAME_ACK,
    FRAME_ID,
    FRAME_NET_STATUS,
    FRAME_READ_TIMEOUT,
    FRAME_UNKNOWN,
    FRAME_SUBSMS_ERROR,
    FRAME_SUBSMS_SEND_STATUS,
    FRAME_SUBSMS_INCOMING,
};
