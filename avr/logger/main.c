#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <util/atomic.h>
#include <util/delay.h>

#define ENABLE_DCIM 0
#define ENABLE_SD 0

#if ENABLE_SD
#include "sd.h"
#include "fat.h"
#endif

#include "irq.h"
#include "uart.h"
#include "spi.h"
#include "mcp2515.h"
#include "time.h"
#include "node.h"
#include "free_ram.h"
#include "can.h"
#include "ircam.h"
#include "debug.h"

#define NUM_LOGS 4
#define LOG_INVALID 0xff
#define LOG_SIZE 512

#define PATH_LEN 128

/*
   NOTE: All callers of SD/FAT functions
   will need to be wrapped in atomic blocks
   to ensure that the CAN interrupt doesn't
   pre-empt reading from the SD card.
   Pre-empting the reading may cause
   the AVR to miss the SPI response from the
   SD card, which will cause it to loop
   infinitely
*/


uint8_t cur_log;
uint32_t tmp_write_pos;
uint32_t log_write_pos;

#if ENABLE_DCIM
struct can_dcim {
    uint16_t dir;
    uint16_t dscf;
    char ext[4];
};

uint8_t tree(uint8_t dest);
#endif

ISR(USART_RX_vect)
{
    uint8_t c;

    c = UDR0;

    if (read_flag)
        return;

    if (read_size < sizeof(rcv_buf) - 1)
        rcv_buf[read_size++] = c;

    if (read_size < target_size)
        return;

    read_flag = 1;
}

#if ENABLE_SD
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

    printwait_P(PSTR("dmp pg bf %u %lu\n"), cur_log, log_write_pos);

    /* if this log file is too full for the current page buf, then switch
       to the next free log file */
    if (((uint32_t)log_write_pos + page_buf_len) > (uint32_t)LOG_SIZE) {
        offer_num = cur_log;
        irq_signal |= IRQ_USER;
        printwait_P(PSTR("nxtlg @ %u\n"), cur_log);
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
        printwait_P(PSTR("er lg op%u\n"), cur_log);
        goto err;
    }

    rc = pf_lseek(log_write_pos);
    if (rc) {
        puts_P(PSTR("er3"));
        goto err;
    }

    rc = pf_write(page_buf, page_buf_len, &wrote);
    printwait_P(PSTR("cb: pf_wr %u = '%u'\n"), page_buf_len, wrote);
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

    snprintf_P(buf, sizeof(buf), PSTR("%u.LEN"), cur_log);
    rc = pf_open(buf);
    if (rc) {
        printwait_P(PSTR("er3 %u\n"), rc);
        goto err;
    }
    printwait_P(PSTR("opened '%s'\n"), buf);

    rc = pf_write(&log_write_pos, sizeof(log_write_pos), &wrote);
    if (rc || wrote != sizeof(log_write_pos)) {
        puts_P(PSTR("er4"));
        goto err;
    }
    printwait_P(PSTR("wrote %d\n"), wrote);

    rc = pf_write(NULL, 0, &wrote);
    if (rc) {
        puts_P(PSTR("er5"));
        goto err;
    }
    printwait_P(PSTR("finalized\n"));

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

    printwait_P(PSTR("log packet\n"));

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

    printwait_P(PSTR("pg bf %u\n"), page_buf_len);

    return 0;
}

volatile uint8_t offer_num;

#endif
uint8_t user_irq(void)
{
#if ENABLE_SD
    char buf[8];
    snprintf_P(buf, sizeof(buf), PSTR("%u.LOG"), offer_num);
    mcp2515_send(TYPE_file_offer, ID_logger, buf, strlen(buf));
    puts_P(PSTR("offered a full log file over CAN"));
#endif
    return 0;
}


void can_tophalf(void)
{
#if ENABLE_SD
    /* we're in interrupt context! */
    uint8_t rc;
    printwait_P(PSTR("tophalf\n"));

    /* don't log transfer chunks */
    if (packet.type == TYPE_xfer_chunk)
        return;

    rc = log_packet();

    if (rc) {
        printwait_P(PSTR("log pkt err\n"));
        /* problem logging packet */
    }
#endif
}

#if ENABLE_DCIM
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
        mcp2515_send(TYPE_format_error, ID_logger, NULL, 0);
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
        mcp2515_send(TYPE_file_error, ID_logger, buf, 8);
        return;
    }

    ptr = (uint8_t *)&packet.data;
    rc = mcp2515_xfer(TYPE_dcim_header, packet.id, ptr, packet.len, 0);
    if (rc)
        return;

    rc = mcp2515_xfer(TYPE_dcim_len, ID_logger, &(fs.fsize), sizeof(fs.fsize), 0);
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
            mcp2515_send(TYPE_file_error, ID_logger, NULL, 0);
            break;
        }

        rc = mcp2515_xfer(TYPE_xfer_chunk, ID_logger, buf, rd, 0);
        printf_P(PSTR("xf %u\n"), rc);
        if (rc) {
            //puts_P(PSTR("xfer failed"));
            break;
        }
    }

    return;
}
#endif

#if ENABLE_SD
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
        mcp2515_send(TYPE_file_error, ID_logger, buf, strlen(buf));
        return;
    }

    rc = mcp2515_xfer(TYPE_file_header, ID_logger, buf, packet.len, 0);
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
            mcp2515_send(TYPE_file_error, ID_logger, NULL, 0);
            break;
        }

        rc = mcp2515_xfer(TYPE_xfer_chunk, ID_logger, buf, rd, 0);
        printf_P(PSTR("xf %u\n"), rc);
        if (rc) {
            //puts_P(PSTR("xfer failed"));
            break;
        }
    }

    return;
}

uint8_t fat_init(void)
{
    uint8_t rc;

    sd_init();

    rc = disk_initialize();
    printwait_P(PSTR("dsk %u\n"), rc);
    if (rc)
        return rc;

    rc = pf_mount();
    printwait_P(PSTR("fs %u\n"), rc);
    if (rc)
        return rc;

    spi_medium();

    return 0;
}
#endif

uint8_t can_irq(void)
{
    uint8_t rc = 0;

    switch (packet.type) {
    case TYPE_ircam_read:
        IRCAM_ON();

        _delay_ms(1000);

        rc = ircam_init_xfer();
        if(rc == 0) {
            rc = ircam_read_fbuf();
            printf("read_fbuf rc = %d\n", rc);
        }

        IRCAM_OFF();

        break;
    case TYPE_ircam_reset:
        ircam_reset();
        break;
    }

    packet.unread = 0;

    return rc;
}

#if ENABLE_SD
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
        rc = mcp2515_xfer(TYPE_xfer_chunk, MY_ID, buf, chunksize, 0);
        if (rc)
            return rc;

        buf += chunksize;
        i -= chunksize;
    }

    return 0;
}

uint8_t tree_send_path(char *path)
{
    uint8_t rc;
    uint8_t len;
    char buf[64];
    DIR dir;
    FILINFO fno;

    printf("path '%s'\n", path);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        rc = pf_opendir(&dir, path);
    }
    printf("x: %d\n", rc);
    if (rc)
        return rc;

    for (;;) {

        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            rc = pf_readdir(&dir, &fno);
        }

        if (rc)
            return rc;

        if (!fno.fname[0])
            break;

        if (fno.fattrib & AM_DIR) {
            len = strlen(path);

            snprintf_P(buf, sizeof(buf), PSTR("%s/\n"), fno.fname);
            rc = tree_send_chunks(buf);
            if (rc)
                return rc;

            snprintf_P(path + len, PATH_LEN - len, PSTR("/%s"), fno.fname);

            rc = tree_send_path(path);
            if (rc)
                return rc;

            rc = mcp2515_xfer(TYPE_xfer_chunk, MY_ID, "\n", 1, 0);
            if (rc)
                return rc;

            path[len] = '\0';
        } else {
            snprintf_P(buf, sizeof(buf), PSTR("%s [%lu]\n"),
                    fno.fname, fno.fsize);

            rc = tree_send_chunks(buf);
            if (rc)
                return rc;
        }

    }

    return 0;
}


uint8_t tree(uint8_t dest)
{
    uint8_t rc;
    char path[PATH_LEN];

    path[0] = '\0';

    rc = tree_send_path(path);
    if (rc)
        return rc;

    rc = mcp2515_xfer(TYPE_xfer_chunk, dest, NULL, 0, 0);
    if (rc)
        return rc;

    return 0;
}
#endif


uint8_t periodic_irq(void)
{
    //puts("tick");

    return 0;
}


uint8_t debug_irq(void)
{
    char buf[32];

    fgets(buf, sizeof(buf), stdin);
    buf[strlen(buf)-1] = '\0';

    debug_flush();

    if (strcmp(buf, "on") == 0) {
        PORTD |= (1 << PORTD7);
    } else if (strcmp(buf, "off") == 0) {
        PORTD &= ~(1 << PORTD7);
    } else if (strcmp(buf, "snap") == 0) {
        ircam_init_xfer();
        ircam_read_fbuf();
    } else if (strcmp(buf, "rst") == 0) {
        printf("reset cam\n");
        ircam_reset();
    } else if (strcmp(buf, "mcp") == 0) {
        mcp2515_dump();
        //mcp2515_init();
    }
#if ENABLE_SD
    else if (buf[0] == 'f' && buf[1] == '\0') {
        printf_P(PSTR("%u/%u\n"), free_ram(), stack_space());
    } else if (buf[0] == '5' && buf[1] == '\0') {
        log_write_pos = 480;
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
    } else {
        putchar('?');
    }
#endif
    else {
        printf("got '%s'\n", buf);
    }

    PROMPT;

    return 0;
}

int main(void)
{
#if ENABLE_SD
    /* clear up the SPI bus for the mcp2515 */
    spi_init();
    sd_hw_init();
#endif

    NODE_INIT();

#if ENABLE_SD
    rc = fat_init();
    if (rc) {
        printwait_P(PSTR("fat err\n"));
        //mcp2515_send(TYPE_file_error, ID_logger, &rc, sizeof(rc));
        sei();
        uart_tx_flush();
        _delay_ms(1000);
        goto reinit;
    }
#endif

    sei();

#if ENABLE_SD
    /* we start searching from log AFTER this one, which is num 0 */
    cur_log = find_free_log_after(NUM_LOGS - 1);


    if (cur_log == LOG_INVALID) {
        /* disk full? */
        puts_P(PSTR("fle err"));
        mcp2515_send(TYPE_disk_full, ID_logger, &rc, sizeof(rc));
        /*
        _delay_ms(1000);
        cli();
        goto reinit;
        */
    }
#endif

    // For the ircam
    DDRD |= (1 << PORTD7);

    NODE_MAIN();
}
