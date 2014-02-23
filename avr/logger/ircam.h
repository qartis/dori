#define IRCAM_ON()  PORTD |=  (1 << PORTD7)
#define IRCAM_OFF() PORTD &= ~(1 << PORTD7)

uint8_t ircam_send_cmd(uint8_t *cmd, uint8_t cmd_nbytes, uint8_t target_nbytes);
void ircam_reset(void);

uint8_t ircam_init_xfer(void);
uint8_t ircam_read_fbuf(void);

#define ACK_SIZE 5
#define DATA_CHUNK_SIZE 8
#define RCV_BUF_SIZE (32 + (ACK_SIZE * 2))

extern volatile uint8_t rcv_buf[RCV_BUF_SIZE];
extern volatile uint8_t read_flag;

// how many bytes ISR should expect to receive
extern volatile uint8_t target_size;
extern volatile uint8_t read_size;

// remaining size of the frame buffer to read from the camera
extern volatile uint32_t fbuf_len;

