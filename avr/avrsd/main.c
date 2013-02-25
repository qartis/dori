#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "sd.h"
#include "fat.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"

FATFS fs;

static void put_rc(FRESULT rc)
{
    FRESULT i;
    const char *PROGMEM p;
    static const char str[] PROGMEM =
        "OK\0" "DSK_ER\0" "NT_RDY\0" "NO_FLE\0" "NO_PTH\0"
        "NO_OPEN\0" "NO_EN\0" "NO_FS\0";

    for (p = str, i = 0; i != rc && pgm_read_byte_near(p); i++) {
        while (pgm_read_byte_near(p++)) ;
    }
    printf("rc=%u FR_%S\n", rc, p);
}

void mcp2515_callback(void)
{
    uint8_t i;

    printf("SD node received [%x %x] %db: ", packet.type, packet.id, packet.len);
    for (i = 0; i < packet.len; i++) {
        printf("%x,", packet.data[i]);
    }
    printf("\n");
}

uint8_t cat(uint8_t dest, uint8_t *filename)
{
    uint8_t rc;
    uint8_t done = 0;
    uint8_t buf[8];
    uint16_t buflen;
    DIR dir;

    rc = pf_open((const char *)filename);
    if (rc) {
        printf("pf_open error: %d\n", rc);
        return 1;
    }

    while (!done) {
        rc = pf_read(buf, 8, &buflen);
        if (rc) {
            printf("pf_read error: %d\n", rc);
            return 2;
        }

        printf("pf_read: %d bytes\n", buflen);

        rc = mcp2515_xfer(TYPE_FILE_CONTENTS, dest, (uint8_t)buflen, buf);
        if (rc) {
            printf("xfer: failed (%d)\n", rc);
            return 3;
        }

        if (buflen < 8)
            done = 1;
    }

    return 0;
}

uint8_t ls(uint8_t dest)
{
    uint8_t rc;
    FILINFO fno;
    DIR dir;
    uint8_t buf[64];
    uint8_t buflen = 0;
    uint8_t done = 0;

    rc = pf_opendir(&dir, "");
    if (rc)
        return 1;

    while (!done) {
        while (buflen < 8) {
            rc = pf_readdir(&dir, &fno);
            if (rc) {
                printf("readdir: %d\n", rc);
                return 2;
            }

            if (!fno.fname[0]) {
                done = 1;
                break;
            }

            memcpy(buf + buflen, fno.fname, strlen(fno.fname));
            buflen += strlen(fno.fname);
            printf("file: %s\n", fno.fname);
            if (fno.fattrib & AM_DIR) {
                buf[buflen++] = '/';
                printf("/");
            } else {
                rc = snprintf((char *)buf + buflen, sizeof(buf) - buflen, " [%ld]", fno.fsize);
                buflen += rc;
                printf(" [%ld]", fno.fsize);
            }
            buf[buflen++] = '\n';
            putchar('\n');
        }

        buf[buflen] = '\0';

        printf("done filling: '%s'\n", buf);

        if (buflen == 0) {
            break;
        }

        unsigned char *ptr = buf;
        while (buflen >= 8) {
            rc = mcp2515_xfer(TYPE_FILE_LISTING, dest, 8, ptr);
            if (rc) {
                printf("xfer: failed (%d)\n", rc);
                return 3;
            }
            ptr += 8;
            buflen -= 8;
        }

        printf("done sending\n");

        memmove(buf, ptr, buflen);
    }

    printf("sending last chunk\n");
    rc = mcp2515_xfer(TYPE_FILE_LISTING, dest, buflen, buf);
    if (rc) {
        printf("xfer: failed\n");
        return 4;
    }

    return 0;
}

void main(void)
{
    struct mcp2515_packet_t p;
    uint8_t rc;

    uart_init(BAUD(38400));
    spi_init();
    sd_init();

    _delay_ms(200);
    printf("init\n");

    while (mcp2515_init()) {
        printf("mcp: 1\n");
        _delay_ms(500);
    }

    for (;;) {
        printf("> ");

        p = mcp2515_get_packet();
        switch (p.type) {
        case TYPE_REQUEST_LIST_FILES:
            sd_init();
            rc = disk_initialize();
            printf("disk: %d\n", rc);
            rc = pf_mount(&fs);
            printf("fs: %d\n", rc);
            rc = ls(p.data[0]);
            printf("ls: %d\n", rc);
            if (rc) {
                mcp2515_send(TYPE_FILE_ERROR, p.data[0], 1, &rc);
                printf("send FILE_ERROR\n");
            }
            break;
        case TYPE_REQUEST_FILE:
            sd_init();
            rc = disk_initialize();
            printf("disk: %d\n", rc);
            rc = pf_mount(&fs);
            printf("fs: %d\n", rc);
            rc = cat(p.data[0], p.data + 1);
            printf("cat: %d\n", rc);
            if (rc) {
                mcp2515_send(TYPE_FILE_ERROR, p.data[0], 1, &rc);
                printf("send FILE_ERROR\n");
            }
            break;
        default:
            printf("unknown CAN message type %x\n", p.type);
            break;
        }
    }
}
