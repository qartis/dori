#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "sd.h"
#include "fat.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "node.h"
#include "debug.h"

#define NUM_LOGS 64
#define LOG_INVALID 0xff
#define LOG_SIZE 100

uint8_t cur_log;
uint32_t tmp_write_pos;
uint32_t log_write_pos;

uint8_t find_free_log_after(uint8_t start)
{
    uint8_t i;
    uint8_t rc;
    char buf[16];
    uint16_t log_len;
    uint16_t discard;

    for (i = (start + 1) % NUM_LOGS; i != start; i = (i + 1) % NUM_LOGS) {
        printf_P(PSTR("trying log %d\n"), i);
        snprintf(buf, sizeof(buf), "%d.LEN", i);

        rc = pf_open(buf);
        if (rc) { /* WTF */
            printf_P(PSTR("open error: %s\n"), buf);
            continue;
        }

        rc = pf_read(&log_len, sizeof(log_len), &discard);
        if (rc) { /* WTF */
            printf_P(PSTR("read error: %s\n"), buf);
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

    printf_P(PSTR("dumping page buf: %u %lu\n"), cur_log, log_write_pos);

    if (((uint32_t)log_write_pos + page_buf_len) > LOG_SIZE) {
        printf_P(PSTR("next log file start %d\n"), cur_log);
        log_write_pos = 0;
        cur_log = find_free_log_after(cur_log);
        if (cur_log == LOG_INVALID) {
            goto err;
        }
    }

    snprintf(buf, sizeof(buf), "%d.LOG", cur_log);
    rc = pf_open(buf);
    if (rc) {
        printf_P(PSTR("error opening log '%s'\n"), buf);
        goto err;
    }

    rc = pf_lseek(log_write_pos);
    if (rc) {
        printf_P(PSTR("err3\n"));
        goto err;
    }

    rc = pf_write(page_buf, page_buf_len, &wrote);
    printf_P(PSTR("cb: pf_write(%d) = '%u'\n"), page_buf_len, wrote);
    if (rc || wrote != page_buf_len) {
        printf_P(PSTR("err1\n"));
        goto err;
    }

    rc = pf_write(NULL, 0, &wrote);
    if (rc) {
        printf_P(PSTR("err2\n"));
        goto err;
    }

    log_write_pos += page_buf_len;
    page_buf_len = 0;

    snprintf(buf, sizeof(buf), "%d.LEN", cur_log);
    rc = pf_open(buf);
    if (rc) {
        printf_P(PSTR("err3 %s: %d\n"), buf, rc);
        goto err;
    }

    rc = pf_write(&log_write_pos, sizeof(log_write_pos), &wrote);
    if (rc || wrote != sizeof(log_write_pos)) {
        printf_P(PSTR("err4\n"));
        goto err;
    }

    rc = pf_write(NULL, 0, &wrote);
    if (rc) {
        printf_P(PSTR("err5\n"));
        goto err;
    }

    return 0;

err:
    page_buf_len = 0;
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
    if (((uint16_t)page_buf_len + packet.len) > sizeof(page_buf)) {
        page_buf[page_buf_len++] = packet.type;
        page_buf[page_buf_len++] = packet.id;
        page_buf[page_buf_len++] = packet.len;

        for (i = 0; i < packet.len; i++) {
            page_buf[page_buf_len++] = packet.data[i];
        }

        return 0;
    }

    /* dump the page buf */
    cli();
    rc = dump_page_buf(page_buf, page_buf_len);
    sei();

    return rc;
}

void can_tophalf(void)
{
    /* we're in interrupt context! */
    uint8_t rc;

    /* don't log transfer chunks */
    if (packet.type == TYPE_xfer_chunk)
        return;

    printf("got packet!\n");
    rc = log_packet();

    if (rc) {
        /* problem logging packet */
    }
}

uint8_t uart_irq(void)
{
    char buf[512];
    uint8_t rc;
    
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';

    printf_P(PSTR("got '%s'\n"), buf);

    if (strcmp(buf, "read") == 0) {
        rc = pf_open("tmp");
        printf("open: %d\n", rc);
        uint16_t rd;
        pf_read(buf, 512, &rd);
        uint16_t i;
        for(i=0;i<rd;i++){
            printf("%x ", buf[i]);
        }
    }

    return 0;
}

uint8_t periodic_irq(void)
{
    puts("tick");

    return 0;
}

uint8_t fat_init(void)
{
    uint8_t rc;

    sd_init();

    rc = disk_initialize();
    printf_P(PSTR("disk: %d\n"), rc);
    if (rc)
        return rc;

    rc = pf_mount();
    printf_P(PSTR("fs: %d\n"), rc);
    if (rc)
        return rc;

    return 0;
}



uint8_t can_irq(void)
{
    packet.unread = 0;

    if (packet.type == TYPE_xfer_cancel && packet.id == MY_ID) {
        xfer_state = XFER_CANCEL;
    } else if (packet.type == TYPE_xfer_cts && packet.id == MY_ID) {
        if (xfer_state == XFER_CHUNK_SENT) {
            xfer_state = XFER_GOT_CTS;
        }
    } else if (packet.type == TYPE_xfer_chunk) {
        if (xfer_state == XFER_WAIT_CHUNK) {
            //xfer_got_chunk();
            /* finish this */
            xfer_state = 0;
        }
    }

    switch (packet.type) {
    case TYPE_file_read:
        
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

    cli();
    if ((page_buf_len + packet.len) > sizeof(page_buf)
        || p.len < 8) {
        /* dump the page buf */

        printf_P(PSTR("dump %lu\n"), tmp_write_pos);

        rc = pf_open("TMP");
        if (rc) {
            rc = 1;
            printf_P(PSTR("TMP err\n"));
            goto err;
        }

        //printf_P(PSTR("seek %lu\n"), tmp_write_pos);
        rc = pf_lseek(tmp_write_pos);
        if (rc) {
            rc = 2;
            printf_P(PSTR("seek er\n"));
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
                        printf_P(PSTR("write single er\n"));
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
                printf_P(PSTR("write final er\n"));
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
        mcp2515_send(TYPE_file_error, MY_ID, sizeof(rc), &rc);
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
        printf_P(PSTR("pf_open er: %d\n"), rc);
        return 1;
    }

    while (!done) {
        rc = pf_read(buf, 8, &buflen);
        if (rc) {
            printf_P(PSTR("pf_read er: %d\n"), rc);
            return 2;
        }

        printf_P(PSTR("pf_read: %db\n"), buflen);

        //rc = mcp2515_xfer(TYPE_file_contents, dest, (uint8_t)buflen, buf);
        if (rc) {
            printf_P(PSTR("xf: er (%d)\n"), rc);
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
                printf_P(PSTR("readdir: %d\n"), rc);
                return 2;
            }

            if (!fno.fname[0]) {
                done = 1;
                break;
            }

            memcpy(buf + buflen, fno.fname, strlen(fno.fname));
            buflen += strlen(fno.fname);
            printf_P(PSTR("file: %s\n"), fno.fname);
            if (fno.fattrib & AM_DIR) {
                buf[buflen++] = '/';
                printf_P(PSTR("/"));
            } else {
                rc = snprintf((char *)buf + buflen, sizeof(buf) - buflen, " [%ld]", fno.fsize);
                buflen += rc;
                printf_P(PSTR(" [%ld]"), fno.fsize);
            }
            buf[buflen++] = '\n';
            putchar('\n');
        }

        buf[buflen] = '\0';

        printf_P(PSTR("done filling: '%s'\n"), buf);

        if (buflen == 0) {
            break;
        }

        unsigned char *ptr = buf;
        while (buflen >= 8) {
            //rc = mcp2515_xfer(TYPE_file_listing, dest, 8, ptr);
            if (rc) {
                printf_P(PSTR("xfer: failed (%d)\n"), rc);
                return 3;
            }
            ptr += 8;
            buflen -= 8;
        }

        printf_P(PSTR("done sending\n"));

        memmove(buf, ptr, buflen);
    }

    printf_P(PSTR("sending last chunk\n"));
    //rc = mcp2515_xfer(TYPE_file_listing, dest, buflen, buf);
    if (rc) {
        printf_P(PSTR("xfer: failed\n"));
        return 4;
    }

    return 0;
}

void main(void)
{
    NODE_INIT();

    /* SD SPI CS */
    DDRD |= (1 << PORTD7);

    rc = fat_init();
    if (rc) {
        puts("fat err\n");
        //mcp2515_send(TYPE_file_error, ID_sd, sizeof(rc), &rc);
        _delay_ms(1000);
        goto reinit;
    }

    /* we start searching from log AFTER this one, which is num 0 */
    cur_log = find_free_log_after(NUM_LOGS - 1);

    sei();

    if (cur_log == LOG_INVALID) {
        /* the heck */
        printf_P(PSTR("log file error\n"));
        mcp2515_send(TYPE_file_error, ID_sd, sizeof(rc), &rc);
        _delay_ms(1000);
        goto reinit;
    }

    NODE_MAIN();
}
