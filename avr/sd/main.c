#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "sd.h"
#include "irq.h"
#include "fat.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "node.h"
#include "debug.h"
#include "free_ram.h"
#include "can.h"

#define NUM_LOGS 64
#define LOG_INVALID 0xff
#define LOG_SIZE 100

uint8_t cur_log;
uint32_t tmp_write_pos;
uint32_t log_write_pos;

struct can_dcim {
    uint16_t dir;
    uint16_t dscf;
    char ext[4];
};

uint8_t tree(uint8_t dest);

uint8_t find_free_log_after(uint8_t start)
{
    uint8_t i;
    uint8_t rc;
    char buf[16];
    uint16_t log_len;
    uint16_t discard;

    for (i = (start + 1) % NUM_LOGS; i != start; i = (i + 1) % NUM_LOGS) {
        printf_P(PSTR("log %d\n"), i);
        snprintf_P(buf, sizeof(buf), PSTR("%d.LEN"), i);

        rc = pf_open(buf);
        if (rc) { /* WTF */
            puts_P(PSTR("op er"));
            continue;
        }

        rc = pf_read(&log_len, sizeof(log_len), &discard);
        if (rc) { /* WTF */
            puts_P(PSTR("rd er"));
            continue;
        }

        if (log_len == 0) {
            return i;
        }
    }

    return LOG_INVALID;
}

uint8_t dump_page_buf(uint8_t *page_buf, uint8_t page_buf_len)
{
    char buf[64];
    uint16_t wrote;
    uint8_t rc;

    printf_P(PSTR("dmp pg bf %u %lu\n"), cur_log, log_write_pos);

    if (((uint32_t)log_write_pos + page_buf_len) > LOG_SIZE) {
        printf_P(PSTR("nxtlg @ %d\n"), cur_log);
        log_write_pos = 0;
        cur_log = find_free_log_after(cur_log);
        if (cur_log == LOG_INVALID) {
            goto err;
        }
    }

    snprintf_P(buf, sizeof(buf), PSTR("%d.LOG"), cur_log);
    rc = pf_open(buf);
    if (rc) {
        puts_P(PSTR("er lg op"));
        goto err;
    }

    rc = pf_lseek(log_write_pos);
    if (rc) {
        puts_P(PSTR("er3"));
        goto err;
    }

    rc = pf_write(page_buf, page_buf_len, &wrote);
    printf_P(PSTR("cb: pf_wr %d = '%u'\n"), page_buf_len, wrote);
    if (rc || wrote != page_buf_len) {
        puts_P(PSTR("er1"));
        goto err;
    }

    rc = pf_write(NULL, 0, &wrote);
    if (rc) {
        puts_P(PSTR("er2"));
        goto err;
    }

    log_write_pos += page_buf_len;

    snprintf_P(buf, sizeof(buf), PSTR("%d.LEN"), cur_log);
    rc = pf_open(buf);
    if (rc) {
        printf_P(PSTR("er3 %d\n"), rc);
        goto err;
    }

    rc = pf_write(&log_write_pos, sizeof(log_write_pos), &wrote);
    if (rc || wrote != sizeof(log_write_pos)) {
        puts_P(PSTR("er4"));
        goto err;
    }

    rc = pf_write(NULL, 0, &wrote);
    if (rc) {
        puts_P(PSTR("er5"));
        goto err;
    }

    return 0;

err:
    return 1;
}

uint8_t log_packet(void)
{
    /* we're in interrupt context */
    uint8_t i;
    uint8_t rc;
    static uint8_t page_buf[512];
    static uint16_t page_buf_len = 0;

    /* if we have room to store in cache, then copy it and we're done */
    if (((uint16_t)page_buf_len + packet.len + 1 + 1 + 1)
            < sizeof(page_buf)) {
        page_buf[page_buf_len++] = packet.type;
        page_buf[page_buf_len++] = packet.id;
        page_buf[page_buf_len++] = packet.len;

        for (i = 0; i < packet.len; i++) {
            page_buf[page_buf_len++] = packet.data[i];
        }

        printf_P(PSTR("pg bf %d\n"), page_buf_len);
				
        return 0;
    }

    /* dump the page buf */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        rc = dump_page_buf(page_buf, page_buf_len);
        page_buf_len = 0;
    }

    return rc;
}

void can_tophalf(void)
{
    /* we're in interrupt context! */
    uint8_t rc;

    return;

    /* don't log transfer chunks */
    if (packet.type == TYPE_xfer_chunk)
        return;

    puts_P(PSTR("got pkg"));
    rc = log_packet();

    if (rc) {
        /* problem logging packet */
    }
}

void dcim_read(void)
{
    uint8_t rc;
    uint16_t rd;
    char buf[strlen("/DCIM/113CANON/IMG_1954.JPG") + 5];
    uint8_t i;

    /* /DCIM/[xxx]CANON/IMG_[yyyy].ZZZ */

    volatile struct can_dcim *dcim;

    /* we don't require the sender to include
        the terminating nul byte on the extension */
#define DCIM_SIZE (sizeof(struct can_dcim) - 1)

    if (packet.len < DCIM_SIZE) {
        mcp2515_send(TYPE_format_error, ID_sd, NULL, 0);
        return;
    }

    dcim = (volatile struct can_dcim *)&packet.data;
    dcim->ext[3] = '\0';

    snprintf(buf, sizeof(buf), "/DCIM/%03uCANON/IMG_%04u.%c%c%c",
            dcim->dir, dcim->dscf,
            dcim->ext[0], dcim->ext[1], dcim->ext[2]);

    rc = pf_open(buf);
    printf_P(PSTR("opn %s %d\n"), buf, rc);
    if (rc) {
        mcp2515_send(TYPE_file_error, ID_sd, buf, 8);
        return;
    }

    rc = mcp2515_xfer(TYPE_dcim_header, ID_sd, &(fs.fsize), sizeof(fs.fsize));
    printf_P(PSTR("xf %d\n"), rc);
    if (rc) {
        //puts_P(PSTR("xfer failed"));
        return;
    }

    _delay_ms(200);

    rd = 1;
    while (rd != 0){
        rc = pf_read(buf, 8, &rd);
        if (rc) {
            puts_P(PSTR("read er"));
            mcp2515_send(TYPE_file_error, ID_sd, NULL, 0);
            break;
        }

        rc = mcp2515_xfer(TYPE_xfer_chunk, ID_sd, buf, rd);
        printf_P(PSTR("xf %d\n"), rc);
        if (rc) {
            //puts_P(PSTR("xfer failed"));
            break;
        }
    }

    return;
}

void file_read(void)
{
    uint8_t rc;
    uint16_t rd;
    char buf[8];
    uint8_t i;

    for (i = 0; i < packet.len; i++)
        buf[i] = packet.data[i];

    buf[packet.len] = '\0';

    rc = pf_open(buf);
    printf_P(PSTR("open %s %d\n"), buf, rc);
    if (rc) {
        mcp2515_send(TYPE_file_error, ID_sd, buf, strlen(buf));
        return;
    }

    /*
    rc = mcp2515_xfer(TYPE_file_header, ID_sd, &(fs.fsize), sizeof(fs.fsize));
    printf_P(PSTR("xf %d\n"), rc);
    if (rc) {
        //puts_P(PSTR("xfer failed"));
        return;
    }

    _delay_ms(200);
    */

    rd = 1;
    while (rd != 0){
        rc = pf_read(buf, 8, &rd);
        if (rc) {
            puts_P(PSTR("read er"));
            mcp2515_send(TYPE_file_error, ID_sd, NULL, 0);
            break;
        }

        rc = mcp2515_xfer(TYPE_xfer_chunk, ID_sd, buf, rd);
        printf_P(PSTR("xf %d\n"), rc);
        if (rc) {
            //puts_P(PSTR("xfer failed"));
            break;
        }
    }

    return;
}

uint8_t uart_irq(void)
{
    char buf[512];
    uint8_t rc;
    uint16_t i;
    
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';

    if (strcmp_P(buf, PSTR("sd read")) == 0) {
        packet.data[0] = 't';
        packet.data[1] = 'm';
        packet.data[2] = 'p';
        packet.len = 3;
        file_read();
    } else if (strcmp_P(buf, PSTR("dcim")) == 0) {
        volatile struct can_dcim *dcim = (volatile void *)&packet.data;
        dcim->dir = 113;
        dcim->dscf = 1234;
        dcim->ext[0] = 'J';
        dcim->ext[1] = 'P';
        dcim->ext[2] = 'G';
        packet.len = 2 + 2 + 3;
        dcim_read();
    } else if (strcmp_P(buf, PSTR("tree")) == 0) {
        uint8_t dest = 0x03;
        rc = tree(dest);
        printf_P(PSTR("tree %d\n"), rc);
    }

    PROMPT;

    return 0;
}

uint8_t periodic_irq(void)
{
    //puts("tick");

    return 0;
}

uint8_t fat_init(void)
{
    uint8_t rc;

    sd_init();

    rc = disk_initialize();
    printf_P(PSTR("dsk %d\n"), rc);
    if (rc)
        return rc;

    rc = pf_mount();
    printf_P(PSTR("fs %d\n"), rc);
    if (rc)
        return rc;

    spi_medium();

    return 0;
}

uint8_t can_irq(void)
{
    packet.unread = 0;

    switch (packet.type) {
    case TYPE_file_read:
        file_read();
        break;
    case TYPE_dcim_read:
        dcim_read();
        break;
    case TYPE_file_write:
        break;
    }

    return 0;
}

uint8_t write_file_cb(void)
{
    uint8_t i;
    uint8_t rc;
    uint16_t wrote;
    static uint8_t page_buf[512];
    static uint16_t page_buf_len = 0;

    struct mcp2515_packet_t p = packet;

    return 0;

    cli();
    if ((page_buf_len + packet.len) > sizeof(page_buf)
        || p.len < 8) {
        /* dump the page buf */

        printf_P(PSTR("dmp %lu\n"), tmp_write_pos);

        rc = pf_open("TMP");
        if (rc) {
            rc = 1;
            puts_P(PSTR("TMP er"));
            goto err;
        }

        //printf_P(PSTR("seek %lu\n"), tmp_write_pos);
        rc = pf_lseek(tmp_write_pos);
        if (rc) {
            rc = 2;
            puts_P(PSTR("sek er"));
            goto err;
        }

        rc = pf_write(page_buf, page_buf_len, &wrote);
        printf_P(PSTR("t wr %d = %u\n"), page_buf_len, wrote);
        if (rc || wrote < page_buf_len) {
            rc = 3;
            printf_P(PSTR("tmp wr er\n"));
            goto err;
        }

        if (p.len == 8) {
            uint8_t page_free = sizeof(page_buf) - page_buf_len;

            for (i = 0; i < packet.len; i++) {
                if (i < page_free) {
                    rc = pf_write(&p.data[i], 1, &wrote);
                    if (rc || wrote < 1) {
                        printf_P(PSTR("wr 1 er\n"));
                        rc = 4;
                        goto err;
                    }
                } else {
                    page_buf[i - page_free] = p.data[i];
                }
            }

            page_buf_len = packet.len - page_free;
            tmp_write_pos += sizeof(page_buf);
        } else {
            rc = pf_write(p.data, p.len, &wrote);
            if (rc || wrote < p.len) {
                printf_P(PSTR("wr fi er\n"));
                rc = 4;
                goto err;
            }
            page_buf_len = 0;
            tmp_write_pos = 0;
        }

        rc = pf_write(NULL, 0, &wrote);
        if (rc) {
            rc = 3;
            printf_P(PSTR("tmp wr fini er\n"));
            goto err;
        }
    } else {
        for (i = 0; i < p.len; i++) {
            page_buf[page_buf_len++] = p.data[i];
        }
    }
    sei();
    return 0;

err:
    sei();
    return rc;
}

uint8_t write_file(uint8_t sender_id, uint32_t offset)
{
    tmp_write_pos = offset;

#if 0
    rc = mcp2515_receive_xfer_wait(TYPE_file_contents, sender_id, write_file_cb);
    if (rc) {
        mcp2515_send(TYPE_file_error, MY_ID, &rc, sizeof(rc));
        printf_P(PSTR("rx_xf_wt er\n"));
        return rc;
    }
#endif

    return 0;
}

uint8_t cat(uint8_t dest, const char *filename)
{
    uint8_t rc;
    uint8_t done = 0;
    char buf[64];
    uint16_t buflen;


    strcpy(buf, "/");
    strcat(buf, filename);

    rc = pf_open((const char *)filename);
    if (rc) {
        printf_P(PSTR("open er: %d\n"), rc);
        return 1;
    }

    while (!done) {
        rc = pf_read(buf, 8, &buflen);
        if (rc) {
            printf_P(PSTR("read er: %d\n"), rc);
            return 2;
        }

        printf_P(PSTR("readb %d\n"), buflen);

        //rc = mcp2515_xfer(TYPE_file_contents, dest, buf, (uint8_t)buflen);
        if (rc) {
            printf_P(PSTR("xf er %d\n"), rc);
            return 3;
        }

        if (buflen < 8)
            done = 1;
    }

    return 0;
}

uint8_t tree(uint8_t dest)
{
    uint8_t rc;
    FILINFO fno;
    uint8_t buf[64];
    uint8_t buflen = 0;
    uint8_t done = 0;
    DIR dir;

    rc = pf_opendir(&dir, "/");
    if (rc)
        return rc;

    while (!done) {
        while (buflen < 8) {
            rc = pf_readdir(&dir, &fno);
            if (rc) {
                printf_P(PSTR("dir %d\n"), rc);
                return 2;
            }

            if (!fno.fname[0]) {
                done = 1;
                break;
            }

            memcpy(buf + buflen, fno.fname, strlen(fno.fname));
            buflen += strlen(fno.fname);
            printf_P(PSTR("file %s\n"), fno.fname);
            if (fno.fattrib & AM_DIR) {
                buf[buflen++] = '/';
                putchar('/');
            } else {
                rc = snprintf_P((char *)buf + buflen, sizeof(buf) - buflen, PSTR(" [%ld]"), fno.fsize);
                buflen += rc;
                printf_P(PSTR(" [%ld]"), fno.fsize);
            }
            buf[buflen++] = '\n';
            putchar('\n');
        }

        buf[buflen] = '\0';

        printf_P(PSTR("done w '%s'\n"), buf);

        if (buflen == 0) {
            break;
        }

        uint8_t *ptr = buf;
        while (buflen >= 8) {
            rc = mcp2515_xfer(TYPE_file_tree, dest, ptr, 8);
            if (rc) {
                printf_P(PSTR("xfer fail %d\n"), rc);
                return 3;
            }
            ptr += 8;
            buflen -= 8;
        }

        printf_P(PSTR("done snd\n"));

        memmove(buf, ptr, buflen);
    }

    printf_P(PSTR("send last cnk\n"));
    rc = mcp2515_xfer(TYPE_file_tree, dest, buf, buflen);
    if (rc) {
        printf_P(PSTR("xfer failed\n"));
        return 4;
    }

    return 0;
}

void main(void)
{
    wdt_disable();
    /* clear up the SPI bus for the mcp2515 */
    spi_init();
    sd_hw_init();

    NODE_INIT();

    rc = fat_init();
    if (rc) {
        puts_P(PSTR("fat err"));
        //mcp2515_send(TYPE_file_error, ID_sd, &rc, sizeof(rc));
        _delay_ms(1000);
        goto reinit;
    }

    /* we start searching from log AFTER this one, which is num 0 */
    cur_log = find_free_log_after(NUM_LOGS - 1);

    sei();

    if (cur_log == LOG_INVALID) {
        /* the heck */
        printf_P(PSTR("file err\n"));
        mcp2515_send(TYPE_file_error, ID_sd, &rc, sizeof(rc));
        _delay_ms(1000);
        cli();
        goto reinit;
    }

    NODE_MAIN();
}
