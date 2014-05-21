#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <util/delay.h>

#include "mcp2515.h"
#include "can.h"
#include "irq.h"
#include "uart.h"
#include "debug.h"
#include "ircam.h"
#include "errno.h"

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

void ircam_reset(void)
{
    uint8_t i;

    uint8_t reset_cmd[] = {
        0x56,
        0x00,
        0x26,
        0x00
    };

    for (i = 0; i < sizeof(reset_cmd); i++)
        uart_putchar(reset_cmd[i]);

    fbuf_len = 0;
}

uint8_t ircam_send_cmd(uint8_t *cmd, uint8_t cmd_nbytes, uint8_t target_nbytes)
{
    uint8_t i;
    uint8_t retry;

    read_flag = 0;
    read_size = 0;
    target_size = target_nbytes;

    for (i = 0; i < cmd_nbytes; i++)
        uart_putchar(cmd[i]);

    /* 10 * 5ms = 50ms */
    retry = 10;
    while (!read_flag && --retry)
        _delay_ms(5);

    return read_flag == 0; // return zero on success
}

uint8_t ircam_init_xfer(void)
{
    uint8_t i;
    uint8_t rc;

    uint8_t stop_fbuf_cmd[] = {
        0x56,
        0x00,
        0x36,
        0x01,
        0x00
    };

    uint8_t fbuf_len_cmd[] = {
        0x56,
        0x00,
        0x34,
        0x01,
        0x00
    };

    ircam_reset();
    _delay_ms(500);

    // 1. Freeze the frame buffer
    rc = ircam_send_cmd(stop_fbuf_cmd, sizeof(stop_fbuf_cmd), 5);
    if (rc) {
        printf("!stop_fbuf\n");
        return ERR_CAM_INIT;
    }

    // 2. Check the frame buffer length
    rc = ircam_send_cmd(fbuf_len_cmd, sizeof(fbuf_len_cmd), 9);
    if (rc) {
        printf("!fbuf_len\n");
        return ERR_CAM_INIT;
    }

    printf("fbuf: ");
    for (i = 0; i < read_size; i++)
        printf("%x ", rcv_buf[i]);

    printf("\n");

    // data length comes in MSB first
    fbuf_len =
        (((uint32_t)rcv_buf[5] << 24) |
         ((uint32_t)rcv_buf[6] << 16) |
         ((uint32_t)rcv_buf[7] <<  8) |
         ((uint32_t)rcv_buf[8] <<  0));

    printf("fbuf_len: %lu\n", fbuf_len);
    return 0;
}

uint8_t ircam_read_fbuf(void)
{
    uint32_t addr = 0;
    uint32_t total_read = 0;

    uint8_t rc = 0;

    if(fbuf_len == 0) {
        printf("no fbuf_len\n");
        return ERR_CAM_INIT;
    }

    // Send the ircam header
    uint8_t data[4];

    data[0] = (fbuf_len >> 24) & 0xFF;
    data[1] = (fbuf_len >> 16) & 0xFF;
    data[2] = (fbuf_len >>  8) & 0xFF;
    data[3] = (fbuf_len >>  0) & 0xFF;

    mcp2515_xfer_begin();

    rc = mcp2515_xfer(TYPE_ircam_header, MY_ID, SENSOR_ircam, data, sizeof(data));
    if (rc) {
        printf("header rc: %d\n", rc);
        return rc;
    }

    // 3. Ask for all the bytes of the photo in 32 byte chunks (then change to 8 byte once 32 works)
    while (fbuf_len > 0) {
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

        rc = ircam_send_cmd(photo_cmd, sizeof(photo_cmd), req_size + (2 * ACK_SIZE));
        if (rc) {
            return ERR_CAM_READ;
        }

        rc = mcp2515_xfer(TYPE_xfer_chunk, MY_ID, SENSOR_ircam, (const char*)&rcv_buf[ACK_SIZE], req_size);
        if (rc) {
            printf("xfer rc: %d\n", rc);
            return rc;
        }

        addr += req_size;
        total_read += req_size;
        fbuf_len -= req_size;
    }

    printf("\nTotal read %lu\n\n", total_read);

    return 0;
}
