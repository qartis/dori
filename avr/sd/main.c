#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
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

#define NUM_LOGS 4
#define LOG_INVALID 0xff
#define LOG_SIZE 512

uint8_t cur_log;
uint32_t tmp_write_pos;
uint32_t log_write_pos;

struct can_dcim {
    uint16_t dir;
    uint16_t dscf;
    char ext[4];
};

uint8_t tree(uint8_t dest);

void empty(void)
{
    char buf[8];
    uint8_t rc;
    uint8_t i;
    uint32_t zero = 0;
    uint16_t wrote;

    for (i = 0; i < NUM_LOGS; i++) {
        snprintf_P(buf, sizeof(buf), PSTR("%u.LEN"), i);

        rc = pf_open(buf);
        if (rc)
            continue;

        rc = pf_write(&zero, sizeof(zero), &wrote);
        if (rc)
            continue;

        rc = pf_write(NULL, 0, &wrote);
        if (rc)
            continue;
    }
}

volatile uint8_t offer_num;

uint8_t user_irq(void)
{
    char buf[8];
    snprintf_P(buf, sizeof(buf), PSTR("%u.LOG"), offer_num);
    mcp2515_send(TYPE_file_offer, ID_sd, buf, strlen(buf));
    puts_P(PSTR("yo"));
    return 0;
}

uint8_t find_free_log_after(uint8_t start)
{
    uint8_t i;
    uint8_t rc;
    char buf[8];
    uint16_t log_len;
    uint16_t discard;

    for (i = (start + 1) % NUM_LOGS; i != start; i = (i + 1) % NUM_LOGS) {
        printf_P(PSTR("log %u\n"), i);
        snprintf_P(buf, sizeof(buf), PSTR("%u.LEN"), i);

        rc = pf_open(buf);
        if (rc) {
            puts_P(PSTR("op er"));
            continue;
        }

        rc = pf_read(&log_len, sizeof(log_len), &discard);
        if (rc) {
            puts_P(PSTR("rd er"));
            continue;
        }

        if (log_len == 0) {
            return i;
        }
    }

    return LOG_INVALID;
}

uint8_t dump_page_buf(uint8_t *page_buf, uint16_t page_buf_len)
{
    char buf[64];
    uint16_t wrote;
    uint8_t rc;

    printf_P(PSTR("dmp pg bf %u %lu\n"), cur_log, log_write_pos);

    if (((uint32_t)log_write_pos + page_buf_len) > (uint32_t)LOG_SIZE) {
        offer_num = cur_log;
        irq_signal |= IRQ_USER;
        printf_P(PSTR("nxtlg @ %u\n"), cur_log);
        log_write_pos = 0;
        cur_log = find_free_log_after(cur_log);
        if (cur_log == LOG_INVALID) {
            reinit = 1;
            return 1;
        }
    }

    snprintf_P(buf, sizeof(buf), PSTR("%u.LOG"), cur_log);
    rc = pf_open(buf);
    if (rc) {
        printf_P(PSTR("er lg op%u\n"), cur_log);
        goto err;
    }

    rc = pf_lseek(log_write_pos);
    if (rc) {
        puts_P(PSTR("er3"));
        goto err;
    }

    rc = pf_write(page_buf, page_buf_len, &wrote);
    printf_P(PSTR("cb: pf_wr %u = '%u'\n"), page_buf_len, wrote);
    if (rc || wrote != page_buf_len) {
        puts_P(PSTR("er1"));
        goto err;
    }

    rc = pf_write(NULL, 0, &wrote);
    if (rc) {
        puts_P(PSTR("er2"));
        goto err;
    }

    snprintf_P(buf, sizeof(buf), PSTR("%u.LEN"), cur_log);
    rc = pf_open(buf);
    if (rc) {
        printf_P(PSTR("er3 %u\n"), rc);
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

    log_write_pos += page_buf_len;

    return 0;

err:
    reinit = 1;
    return 1;
}

uint8_t log_packet(void)
{
    /* we're in interrupt context */
    uint16_t i;
    uint8_t rc;
    static uint8_t page_buf[512];
    static uint16_t page_buf_len = 0;

    /* if we have room to store in cache, then copy it and we're done */
    if (((uint16_t)page_buf_len + packet.len + 1 + 1 + 1) >= 512) {
        for (i = page_buf_len; i < sizeof(page_buf); i++) {
            page_buf[i] = TYPE_nop;
        }

        /* dump the page buf */
        rc = dump_page_buf(page_buf, sizeof(page_buf));
        if (rc)
            return rc;

        page_buf_len = 0;
    }

    page_buf[page_buf_len++] = packet.type;
    page_buf[page_buf_len++] = packet.id;
    page_buf[page_buf_len++] = packet.len;

    for (i = 0; i < packet.len; i++) {
        page_buf[page_buf_len++] = packet.data[i];
    }

    printf_P(PSTR("pg bf %u\n"), page_buf_len);

    return 0;
}

void can_tophalf(void)
{
    /* we're in interrupt context! */
    uint8_t rc;

    /* don't log transfer chunks */
    if (packet.type == TYPE_xfer_chunk)
        return;

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
    uint8_t *ptr;

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

    snprintf_P(buf, sizeof(buf), PSTR("/DCIM/%03uCANON/IMG_%04u.%c%c%c"),
            dcim->dir, dcim->dscf,
            dcim->ext[0], dcim->ext[1], dcim->ext[2]);

    rc = pf_open(buf);
    printf_P(PSTR("opn %s %u\n"), buf, rc);
    if (rc) {
        mcp2515_send(TYPE_file_error, ID_sd, buf, 8);
        return;
    }

    ptr = (uint8_t *)&packet.data;
    rc = mcp2515_xfer(TYPE_dcim_header, packet.id, ptr, packet.len);
    if (rc)
        return;

    rc = mcp2515_xfer(TYPE_dcim_len, ID_sd, &(fs.fsize), sizeof(fs.fsize));
    printf_P(PSTR("xf %u\n"), rc);
    if (rc) {
        //puts_P(PSTR("xfer failed"));
        return;
    }

    _delay_ms(200);

    rd = 1;
    while (rd != 0) {
        rc = pf_read(buf, 8, &rd);
        if (rc) {
            puts_P(PSTR("read er"));
            mcp2515_send(TYPE_file_error, ID_sd, NULL, 0);
            break;
        }

        rc = mcp2515_xfer(TYPE_xfer_chunk, ID_sd, buf, rd);
        printf_P(PSTR("xf %u\n"), rc);
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
    printf_P(PSTR("open %s %u\n"), buf, rc);
    if (rc) {
        mcp2515_send(TYPE_file_error, ID_sd, buf, strlen(buf));
        return;
    }

    rc = mcp2515_xfer(TYPE_file_header, ID_sd, buf, packet.len);
    printf_P(PSTR("header %u\n"), rc);
    if (rc) {
        //puts_P(PSTR("xfer failed"));
        return;
    }

    rd = 1;
    while (rd != 0) {
        rc = pf_read(buf, 8, &rd);
        if (rc) {
            puts_P(PSTR("read er"));
            mcp2515_send(TYPE_file_error, ID_sd, NULL, 0);
            break;
        }

        rc = mcp2515_xfer(TYPE_xfer_chunk, ID_sd, buf, rd);
        printf_P(PSTR("xf %u\n"), rc);
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
    
    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf) - 1] = '\0';

    if (buf[0] == 'f' && buf[1] == '\0') {
        printf_P(PSTR("%u/%u\n"), free_ram(), stack_space());
    } else if (buf[0] == 'e' && buf[1] == '\0') {
        cli();
        empty();
        sei();
    } else if (buf[0] == '?' && buf[1] == '\0') {
        printf_P(PSTR("wtf %x %x\n"), PCICR, PCMSK0);
    } else if (buf[0] == 's' && buf[1] == 'd' && buf[2] == '\0') {
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
    } else if (buf[0] == 't' && buf[1] == '\0') {
        uint8_t dest = ID_any;
        rc = tree(dest);
        printf_P(PSTR("tree %u\n"), rc);
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
    printf_P(PSTR("dsk %u\n"), rc);
    if (rc)
        return rc;

    rc = pf_mount();
    printf_P(PSTR("fs %u\n"), rc);
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
    case TYPE_file_tree:
        tree(ID_sd);
        break;
    case TYPE_file_write:
        break;
    }

    return 0;
}

uint8_t tree_send_chunks(const char *buf)
{
    uint8_t i;
    uint8_t chunksize;
    uint8_t rc;

    i = strlen(buf);

    while (i > 0) {
        if (i > 8)
            chunksize = 8;
        else
            chunksize = i;

        putchar('\'');
        uint8_t j;
        for (j = 0; j < chunksize; j++) {
            putchar(buf[j]);
        }
        putchar('\'');
        putchar('\n');
        rc = mcp2515_xfer(TYPE_xfer_chunk, MY_ID, buf, chunksize);
        if (rc)
            return 3;

        buf += chunksize;
        i -= chunksize;
    }

    return 0;
}

uint8_t tree_send_path(char *path)
{
    uint8_t rc;
    uint8_t len;
    char buf[32];
    DIR dir;
    FILINFO fno;

    printf("path %s\n", path);

    rc = pf_opendir(&dir, path);
    if (rc)
        return rc;

    for (;;) {
        rc = pf_readdir(&dir, &fno);
        if (rc)
            return 2;

        if (!fno.fname[0])
            break;

        if (fno.fattrib & AM_DIR) {
            len = strlen(path);

            sprintf_P(buf, PSTR("%s/\n"), fno.fname);
            tree_send_chunks(buf);

            sprintf_P(path + len, PSTR("/%s"), fno.fname);

            rc = tree_send_path(path);
            if (rc)
                return rc;

            mcp2515_send(TYPE_xfer_chunk, MY_ID, "\n", 1);

            path[len] = '\0';
        } else {
            snprintf_P(buf, sizeof(buf), PSTR("%s [%lu]\n"),
                    fno.fname, fno.fsize);
            tree_send_chunks(buf);
        }
    }

    return 0;
}


uint8_t tree(uint8_t dest)
{
    uint8_t rc;
    char path[24];

    path[0] = '\0';

    rc = tree_send_path(path);
    if (rc)
        return rc;

    rc = mcp2515_xfer(TYPE_xfer_chunk, dest, NULL, 0);
    if (rc)
        return 4;

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
        /* disk full? */
        puts_P(PSTR("fle err"));
        mcp2515_send(TYPE_disk_full, ID_sd, &rc, sizeof(rc));
        /*
        _delay_ms(1000);
        cli();
        goto reinit;
        */
    }

    NODE_MAIN();
}
