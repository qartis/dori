#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include "sd.h"
#include "fat.h"
//#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"

#define NUM_LOGS 64
#define LOG_INVALID 0xff
#define LOG_SIZE 100

char dir_buf[64];
uint8_t cur_log;
uint32_t tmp_write_pos;
uint32_t log_write_pos;

void periodic_callback(void)
{
    (void)0;
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

    dir_buf[0] = '\0';

    return 0;
}

uint8_t find_free_log_after(uint8_t start)
{
    uint8_t i;
    uint8_t rc;
    char buf[16];
    uint16_t log_len;
    uint16_t discard;

    for (i = (start + 1) % NUM_LOGS; i != start; i = (i + 1) % NUM_LOGS) {
        printf_P(PSTR("Trying %d\n"), i);
        //snprintf(buf, sizeof(buf), "%d.LEN", i);
        rc = pf_open(buf);
        if (rc) { /* WTF */
            printf_P(PSTR("open lg er %s\n"), buf);
            continue;
        }

        rc = pf_read(&log_len, sizeof(log_len), &discard);
        if (rc) { /* WTF */
            printf_P(PSTR("open lg len er %s\n"), buf);
            continue;
        }

        if (log_len == 0) {
            return i;
        }
    }

    return LOG_INVALID;
}

/*
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
    printf_P(PSTR("rc=%u FR_%S\n", rc, p);
}
*/

uint8_t cd(void)
{
    uint8_t rc;
    DIR dir;

    struct mcp2515_packet_t p = packet;
    const char *dirname = (const char *)p.data;

    if (dirname[0] == '/' && dirname[1] == '\0') {
        dir_buf[0] = '\0';
    } else {
        uint8_t len = strlen(dir_buf);
        dir_buf[len] = '/';
        strcpy(dir_buf + len + 1, dirname);
    }

    rc = pf_opendir(&dir, dir_buf);
    if (rc) {
        dir_buf[0] = '\0';
        return rc;
    }

    printf_P(PSTR("dir: '%s'\n"), dir_buf);

    return 0;
}


void mcp2515_irq_callback(void)
{
    uint8_t i;
    static uint8_t page_buf[512];
    static uint16_t page_buf_len = 0;
    uint16_t wrote;
    char buf[16];
    uint8_t rc;

    switch (packet.type) {
    case TYPE_FILE_CHDIR:
        rc = cd();
        printf_P(PSTR("cd: %d\n"), rc);
        break;
    }

    printf_P(PSTR("SD node rx [%x %x] %db: "), packet.type, packet.id, packet.len);
    for (i = 0; i < packet.len; i++) {
        printf_P(PSTR("%x,"), packet.data[i]);
    }
    printf_P(PSTR("\n"));

    if (((uint16_t)page_buf_len + packet.len) > sizeof(page_buf)) {
        /* dump the page buf */

        printf_P(PSTR("dumping page buf: %u %lu\n"), cur_log, log_write_pos);

        cli();

        if (((uint32_t)log_write_pos + page_buf_len) > LOG_SIZE) {
            printf_P(PSTR("next log file start %d\n"), cur_log);
            log_write_pos = 0;
            cur_log = find_free_log_after(cur_log);
            if (cur_log == LOG_INVALID) {
                goto err;
            }
        }

        //snprintf(buf, sizeof(buf), "%d.LOG", cur_log);
        rc = pf_open(buf);
        if (rc) {
            printf_P(PSTR("error opening log '%s'\n"), buf);
            goto err;
        }

        rc = pf_lseek(log_write_pos);
        if (rc) {
            printf_P(PSTR("tmp lseek failed\n"));
            goto err;
        }

        rc = pf_write(page_buf, page_buf_len, &wrote);
        printf_P(PSTR("cb: pf_write(%d) = '%u'\n"), page_buf_len, wrote);
        if (rc || wrote != page_buf_len) {
            printf_P(PSTR("tmp write fialed\n"));
            goto err;
        }

        rc = pf_write(NULL, 0, &wrote);
        if (rc) {
            printf_P(PSTR("final tmp write fialed\n"));
            goto err;
        }

        log_write_pos += page_buf_len;
        page_buf_len = 0;

        //snprintf(buf, sizeof(buf), "%d.LEN", cur_log);
        rc = pf_open(buf);
        if (rc) {
            printf_P(PSTR("error opening log len '%s': %d\n"), buf, rc);
            goto err;
        }

        rc = pf_write(&log_write_pos, sizeof(log_write_pos), &wrote);
        if (rc || wrote != sizeof(log_write_pos)) {
            printf_P(PSTR("log len write failed\n"));
            goto err;
        }

        rc = pf_write(NULL, 0, &wrote);
        if (rc) {
            printf_P(PSTR("final log len w fialed\n"));
            goto err;
        }

        sei();
    } else {
        page_buf[page_buf_len++] = packet.type;
        page_buf[page_buf_len++] = packet.id;
        page_buf[page_buf_len++] = packet.len;
        for (i = 0; i < packet.len; i++) {
            page_buf[page_buf_len++] = packet.data[i];
        }
    }

err:
    sei();
    page_buf_len = 0;
    return;
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

        printf_P(PSTR("dumping tmp buf: %lu\n"), tmp_write_pos);

        rc = pf_open("TMP");
        if (rc) {
            rc = 1;
            printf_P(PSTR("TMP open er\n"));
            goto err;
        }

        printf_P(PSTR("seek(%lu)\n"), tmp_write_pos);
        rc = pf_lseek(tmp_write_pos);
        if (rc) {
            rc = 2;
            printf_P(PSTR("tmp lseek er\n"));
            goto err;
        }

        rc = pf_write(page_buf, page_buf_len, &wrote);
        printf_P(PSTR("tmp: wr(%d) = %u\n"), page_buf_len, wrote);
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
    uint8_t rc;

    tmp_write_pos = offset;

    rc = mcp2515_receive_xfer_wait(TYPE_FILE_CONTENTS, sender_id, write_file_cb);
    if (rc) {
        mcp2515_send(TYPE_FILE_ERROR, MY_ID, sizeof(rc), &rc);
        printf_P(PSTR("rx_xf_wt er\n"));
        return rc;
    }

    return 0;
}

uint8_t cat(uint8_t dest, const char *filename)
{
    uint8_t rc;
    uint8_t done = 0;
    char buf[64];
    uint16_t buflen;

    strcpy(buf, dir_buf);
    strcat(buf, "/");
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

        rc = mcp2515_xfer(TYPE_FILE_CONTENTS, dest, (uint8_t)buflen, buf);
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

    rc = pf_opendir(&dir, dir_buf);
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
                //rc = snprintf((char *)buf + buflen, sizeof(buf) - buflen, " [%ld]", fno.fsize);
                buflen += rc;
                printf_P(PSTR(" [%ld]"), fno.fsize);
            }
            buf[buflen++] = '\n';
            //putchar('\n');
        }

        buf[buflen] = '\0';

        printf_P(PSTR("done filling: '%s'\n"), buf);

        if (buflen == 0) {
            break;
        }

        unsigned char *ptr = buf;
        while (buflen >= 8) {
            rc = mcp2515_xfer(TYPE_FILE_LISTING, dest, 8, ptr);
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
    rc = mcp2515_xfer(TYPE_FILE_LISTING, dest, buflen, buf);
    if (rc) {
        printf_P(PSTR("xfer: failed\n"));
        return 4;
    }

    return 0;
}


void main(void)
{
    uint8_t rc;
    uint8_t sender;
    uint32_t offset;
    struct mcp2515_packet_t p;

    wdt_disable();

    //uart_init(BAUD(38400));
    spi_init();
    time_init();

    _delay_ms(200);
    printf_P(PSTR("sd init\n"));


reinit:
    rc = mcp2515_init();
    if (rc) {
        printf_P(PSTR("mcp: error\n"));
        _delay_ms(1000);
        goto reinit;
    }

    rc = fat_init();
    if (rc) {
        printf_P(PSTR("fat: error\n"));
        mcp2515_send(TYPE_FILE_ERROR, ID_LOGGER, sizeof(rc), &rc);
        _delay_ms(1000);
        goto reinit;
    }

    /* we start searching from log AFTER this one, which is num 0 */
    cur_log = find_free_log_after(NUM_LOGS - 1);
    if (cur_log == LOG_INVALID) {
        /* the heck */
        printf_P(PSTR("log file error\n"));
        mcp2515_send(TYPE_FILE_ERROR, ID_LOGGER, sizeof(rc), &rc);
        _delay_ms(1000);
        goto reinit;
    }

    for (;;) {
        printf_P(PSTR("> "));

        rc = mcp2515_send_xfer_wait(&p);
        if (rc) {
            printf_P(PSTR("sd_xf_wt: er\n"));
            goto reinit;
        }

        printf_P(PSTR("sd_xf_wt: ok\n"));

        switch (p.type) {
        case TYPE_REQUEST_LIST_FILES:
            rc = ls(p.data[0]);
            printf_P(PSTR("ls: %d\n"), rc);
            if (rc) {
                mcp2515_send(TYPE_FILE_ERROR, p.data[0], sizeof(rc), &rc);
            }
            break;
        case TYPE_REQUEST_FILE:
            rc = cat(p.data[0], (const char *)p.data + 1);
            printf_P(PSTR("cat: %d\n"), rc);
            if (rc) {
                mcp2515_send(TYPE_FILE_ERROR, p.data[0], sizeof(rc), &rc);
            }
            break;
        case TYPE_FILE_WRITE:
            printf_P(PSTR("write_file()\n"));
            sender = p.data[0];
            offset = (uint32_t)p.data[1] << 24 |
                     (uint32_t)p.data[2] << 16 |
                     (uint32_t)p.data[3] << 8  |
                     (uint32_t)p.data[4] << 0;
            rc = write_file(sender, offset);
            printf_P(PSTR("write: %d\n"), rc);
            if (rc) {
                mcp2515_send(TYPE_FILE_ERROR, p.data[0], sizeof(rc), &rc);
            }
            break;
        default:
            printf_P(PSTR("unk CAN msg tp %x\n"), p.type);
            break;
        }
    }
}
