#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#include "sd.h"
#include "spi.h"

#define SD_DESELECT() PORTD |=  (1 << PORTD7)
#define SD_SELECT()   PORTD &= ~(1 << PORTD7)

/* Definitions for MMC/SDC command */
#define CMD0    (0x40+0)    /* GO_IDLE_STATE */
#define CMD1    (0x40+1)    /* SEND_OP_COND (MMC) */
#define ACMD41  (0xC0+41)   /* SEND_OP_COND (SDC) */
#define CMD8    (0x40+8)    /* SEND_IF_COND */
#define CMD16   (0x40+16)   /* SET_BLOCKLEN */
#define CMD17   (0x40+17)   /* READ_SINGLE_BLOCK */
#define CMD24   (0x40+24)   /* WRITE_BLOCK */
#define CMD55   (0x40+55)   /* APP_CMD */
#define CMD58   (0x40+58)   /* READ_OCR */

#define GO_IDLE_STATE            0x40|0
#define SEND_OP_COND             0x40|1
#define SET_BLOCK_LEN            0x40|16
#define CRC_ON_OFF               0x40|59

/* Card type flags (CardType) */
#define CT_MMC              0x01    /* MMC ver 3 */
#define CT_SD1              0x02    /* SD ver 1 */
#define CT_SD2              0x04    /* SD ver 2 */
#define CT_BLOCK            0x08    /* Block addressing */

static uint8_t CardType;

void sd_hw_init(void)
{
    DDRD |= (1 << PORTD7);
    SD_DESELECT();
}

void sd_init(void)
{
    spi_low();
}

uint8_t send_cmd (
    uint8_t cmd,        /* 1st byte (Start + Index) */
    uint32_t arg)       /* Argument (32 bits) */
{
    uint8_t n, res;

    if (cmd & 0x80) {   /* ACMD<n> is the command sequence of CMD55-CMD<n> */
        cmd &= 0x7F;
        res = send_cmd(CMD55, 0);
        if (res > 1) return res;
    }

    SD_DESELECT();
    SD_SELECT();

    /* Send a command packet */
    spi_write(cmd);                     /* Start + Command index */
    spi_write((uint8_t)(arg >> 24));        /* Argument[31..24] */
    spi_write((uint8_t)(arg >> 16));        /* Argument[23..16] */
    spi_write((uint8_t)(arg >> 8));         /* Argument[15..8] */
    spi_write((uint8_t)arg);                /* Argument[7..0] */
    n = 0x01;                           /* Dummy CRC + Stop */
    if (cmd == CMD0) n = 0x95;          /* Valid CRC for CMD0(0) */
    if (cmd == CMD8) n = 0x87;          /* Valid CRC for CMD8(0x1AA) */
    spi_write(n);

    /* Wait for a valid command response in timeout of 10 attempts */
    n = 10;
    do {
        res = spi_recv();
    } while ((res & 0x80) && --n);

    return res;
}

uint8_t disk_initialize(void)
{
    uint8_t i, response, retry;

    retry = 255;
    do {
        for (i = 0; i < 10; i++)
            spi_recv();
        response = send_cmd(GO_IDLE_STATE, 0);
        if (--retry == 0)
            return 1;
    } while (response != 0x01);

    spi_recv(); 

    retry = 255;
    do {
        response = send_cmd(SEND_OP_COND, 0); //activate card's initialization process
        if (--retry == 0)
            return 1;
    } while (response);

    send_cmd(CRC_ON_OFF, 0); //disable CRC
    send_cmd(SET_BLOCK_LEN, 512); //set block size to 512

    return 0;
}

#if 0
uint8_t disk_initialize (void)
{
    uint8_t n, cmd, ty, ocr[4];
    uint16_t tmr;

    SD_DESELECT();
    for (n = 10; n; n--) spi_recv();    /* 80 dummy clocks with CS=H */

    ty = 0;
    if (send_cmd(CMD0, 0) == 1) {           /* Enter Idle state */
        if (send_cmd(CMD8, 0x1AA) == 1) {   /* SDv2 */
            for (n = 0; n < 4; n++) ocr[n] = spi_recv();        /* Get trailing return value of R7 resp */
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {         /* The card can work at vdd range of 2.7-3.6V */
                for (tmr = 10000; tmr && send_cmd(ACMD41, 1UL << 30); tmr--) _delay_us(50); /* Wait for leaving idle state (ACMD41 with HCS bit) */
                if (tmr && send_cmd(CMD58, 0) == 0) {       /* Check CCS bit in the OCR */
                    for (n = 0; n < 4; n++) ocr[n] = spi_recv();
                    ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;  /* SDv2 (HC or SC) */
                }
            }
        } else {                            /* SDv1 or MMCv3 */
            if (send_cmd(ACMD41, 0) <= 1)   {
                ty = CT_SD1; cmd = ACMD41;  /* SDv1 */
            } else {
                ty = CT_MMC; cmd = CMD1;    /* MMCv3 */
            }
            for (tmr = 10000; tmr && send_cmd(cmd, 0); tmr--) _delay_us(50);    /* Wait for leaving idle state */
            if (!tmr || send_cmd(CMD16, 512) != 0)          /* Set R/W block length to 512 */
                ty = 0;
        }
    }
    CardType = ty;
    SD_DESELECT();
    spi_recv();

    return ty ? 0 : STA_NOINIT;
}
#endif

/*-----------------------------------------------------------------------*/
/* Read partial sector                                                   */
/*-----------------------------------------------------------------------*/

disk_result_t disk_readp (
    uint8_t *buf,       /* Pointer to the read buffer */
    uint32_t lba,       /* Sector number (LBA) */
    uint16_t ofs,       /* Byte offset to read from (0..511) */
    uint16_t cnt        /* Number of bytes to read (ofs + cnt mus be <= 512) */
)
{
    disk_result_t res;
    uint8_t rc;
    uint16_t bc;


    if (!(CardType & CT_BLOCK)) lba *= 512;     /* Convert to byte address if needed */

    res = RES_ERROR;
    if (send_cmd(CMD17, lba) == 0) {        /* READ_SINGLE_BLOCK */

        bc = 40000;
        do {                            /* Wait for data packet */
            rc = spi_recv();
        } while (rc == 0xFF && --bc);

        if (rc == 0xFE) {               /* A data packet arrived */
            bc = 514 - ofs - cnt;

            /* Skip leading bytes */
            if (ofs) {
                do spi_recv(); while (--ofs);
            }

            do {
                *buf++ = spi_recv();
            } while (--cnt);

            /* Skip trailing bytes and CRC */
            do {
                spi_recv();
            } while (--bc);

            res = RES_OK;
        }
    }

    SD_DESELECT();
    spi_recv();

    return res;
}

disk_result_t disk_writep (
    const uint8_t *buff,   /* Pointer to the bytes to be written (NULL:Initiate/Finalize sector write) */
    uint32_t sa            /* Number of bytes to send, Sector number (LBA) or zero */
)
{
    disk_result_t res;
    uint16_t bc;
    static uint16_t wc;

    res = RES_ERROR;

    if (buff) {     /* Send data bytes */
        bc = (uint16_t)sa;
        while (bc && wc) {      /* Send data bytes to the card */
            spi_write(*buff++);
            wc--; bc--;
        }
        res = RES_OK;
    } else {
        if (sa) {   /* Initiate sector write process */
            if (!(CardType & CT_BLOCK)) sa *= 512;  /* Convert to byte address if needed */
            if (send_cmd(CMD24, sa) == 0) {         /* WRITE_SINGLE_BLOCK */
                spi_write(0xFF); spi_write(0xFE);     /* Data block header */
                wc = 512;                           /* Set byte counter */
                res = RES_OK;
            }
        } else {    /* Finalize sector write process */
            bc = wc + 2;
            while (bc--) spi_write(0);   /* Fill left bytes and CRC with zeros */
            if ((spi_recv() & 0x1F) == 0x05) {   /* Receive data resp and wait for end of write process in timeout of 500ms */
                for (bc = 5000; spi_recv() != 0xFF && bc; bc--) _delay_us(100); /* Wait ready */
                if (bc) res = RES_OK;
            }
            SD_DESELECT();
            spi_recv();
        }
    }

    return res;
}
