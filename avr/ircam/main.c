#include <stdio.h>
#include <string.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include "debug.h"
#include "time.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "node.h"
#include "can.h"
#include "irq.h"

#define ACK_SIZE 5
#define DATA_CHUNK_SIZE 8 
#define RCV_BUF_SIZE (32 + (ACK_SIZE * 2))

#define MIN(a,b) ((a) < (b) ? (a) : (b))

volatile uint8_t rcv_buf[RCV_BUF_SIZE];

volatile uint8_t read_flag;

// how many bytes ISR should expect to receive
volatile uint8_t target_size;
volatile uint8_t read_size;

// remaining size of the frame buffer to read from the camera
volatile uint32_t fbuf_len;


int strstart_P(const char *s1, const char * PROGMEM s2)
{
    return strncmp_P(s1, s2, strlen_P(s2)) == 0;
}


extern volatile uint8_t uart_ring[UART_BUF_SIZE];
extern volatile uint8_t ring_in;
extern volatile uint8_t ring_out;
#define UDR UDR0

ISR(USART_RX_vect)
{
    uint8_t data = UDR;

    if(read_flag)
        return;

    rcv_buf[read_size++] = data;

    if(read_size < target_size) {
        return;
    }

    uart_ring[ring_in] = data;
    ring_in++;
    ring_in %= UART_BUF_SIZE;

    ring_in = ring_out = 0;
    read_flag = 1;
}

uint8_t periodic_irq(void)
{
    print("uart\n");
    printf("debug\n");
    return 0;
}

void reset_cam(void)
{
    uint8_t reset_cmd[] = {
        0x56,
        0x00,
        0x26,
        0x00
    };

    uint8_t i;
    for(i = 0; i < sizeof(reset_cmd); i++) {
        uart_putchar(reset_cmd[i]);
    }
}

uint8_t send_cmd(uint8_t *cmd, uint8_t cmd_nbytes, uint8_t target_nbytes)
{
    uint8_t i;
    uint8_t retry;

    ring_in = ring_out = 0;
    read_flag = 0;
    read_size = 0;
    target_size = target_nbytes;

    //printf("cmd:");
    for(i = 0; i < cmd_nbytes; i++) {
        //printf("%02x ", cmd[i]);
        uart_putchar(cmd[i]);
    }
    //printf("\n");
    retry = 10;
    while(!read_flag && --retry) {
        _delay_ms(5);
    }

    return !read_flag; // return zero on success
}


void init_xfer(void)
{
    reset_cam();
    _delay_ms(500);

    uint8_t i;
    uint8_t rc;
    uint8_t stop_fbuf_cmd[] = {
        0x56,
        0x00,
        0x36,
        0x01,
        0x00
    };

    // 1. Freeze the frame buffer
    rc = send_cmd(stop_fbuf_cmd, sizeof(stop_fbuf_cmd), 5);
    if(rc) {
        printf("!stop_fbuf\n");
        return;
    }

    uint8_t fbuf_len_cmd[] = {
        0x56,
        0x00,
        0x34,
        0x01,
        0x00
    };

    // 2. Check the frame buffer length
    rc = send_cmd(fbuf_len_cmd, sizeof(fbuf_len_cmd), 9);
    if(rc) {
        printf("!fbuf_len\n");
        return;
    }

    printf("fbuf: ");
    for(i = 0; i < read_size; i++) {
        printf("%x ", rcv_buf[i]);
    }

    printf("\n");
    // data length comes in MSB first
    fbuf_len =
        (((uint32_t)rcv_buf[5] << 24) |
         ((uint32_t)rcv_buf[6] << 16) |
         ((uint32_t)rcv_buf[7] <<  8) |
         ((uint32_t)rcv_buf[8] <<  0));

    printf("fbuf_len: %d\n", fbuf_len);
}

uint8_t img_send_chunks(void)
{
    static uint32_t addr = 0;
    static uint32_t total_read = 0;

    uint8_t rc = 0;

    // 3. Ask for all the bytes of the photo in 32 byte chunks (then change to 8 byte once 32 works)
    while(fbuf_len > 0) {
        uint8_t req_size = MIN(DATA_CHUNK_SIZE, fbuf_len);
        //printf("req %d @ addr %d\n", req_size, addr);

        uint8_t photo_cmd[] = {
            0x56, // start a command
            0x00, // serial number (0 is fine)
            0x32, // read fbuf command
            0x0C, // constant
            0x00, // read current frame (0x01 is read next frame)
            0x0A, // 1111 101X where X = {0 = MCU Mode, 1 = DMA mode}

                (addr >> 24) & 0xFF, // address
                (addr >> 16) & 0xFF,
                (addr >>  8) & 0xFF,
                (addr >>  0) & 0xFF,

                ((uint32_t)DATA_CHUNK_SIZE >> 24) & 0xFF, // data size
                ((uint32_t)DATA_CHUNK_SIZE >> 16) & 0xFF,
                ((uint32_t)DATA_CHUNK_SIZE >>  8) & 0xFF,
                ((uint32_t)DATA_CHUNK_SIZE >>  0) & 0xFF,

                0x00, // delay
                0x10
        };

        rc = send_cmd(photo_cmd, sizeof(photo_cmd), req_size + (2 * ACK_SIZE));
        if(rc) {
            printf("!photo_cmd\n");
            return 2;
        }

        rc = mcp2515_xfer(TYPE_ircam_chunk, MY_ID, (const char*)&rcv_buf[ACK_SIZE], req_size, 0);

        if(rc)
            return rc;

        addr += req_size;
        total_read += req_size;
        fbuf_len -= req_size;
    }

    printf("\nTotal read %d\n\n", total_read);

    return 0;
}

uint8_t can_irq(void)
{
    uint8_t rc = 0;

    switch(packet.type) {
    case TYPE_ircam_read:
        init_xfer();
        img_send_chunks();
        break;
    case TYPE_xfer_cancel:
        fbuf_len = 0;
        break;
    case TYPE_ircam_reset:
        reset_cam();
        break;
    }

    packet.unread = 0;
    return rc;
}

uint8_t debug_irq(void)
{
    char buf[32];

    fgets(buf, sizeof(buf), stdin);
    debug_flush();
    buf[strlen(buf)-1] = '\0';

    if(strcmp(buf, "snap") == 0) {
        init_xfer();
        img_send_chunks();
    } else if(strcmp(buf, "rst") == 0) {
        printf("reset cam\n");
        reset_cam();
    }else {
        printf("got '%s'\n", buf);
    }

    return 0;
}

uint8_t uart_irq(void)
{
    return 0;
}

void main(void)
{
    NODE_INIT();
    sei();

    NODE_MAIN();
}
