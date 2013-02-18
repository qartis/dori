#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "uart.h"

#define PAGE_SIZE 128
#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED_SMALL_BIT (1<<PORTB0)
#define LED_BIG_BIT (1<<PORTB1)

uint8_t instruction_buffer[PAGE_SIZE];
uint8_t led_small_asm[PAGE_SIZE];
uint8_t led_big_asm[PAGE_SIZE];

void flash_write_page(uint16_t start, uint8_t * page) BOOTLOADER_SECTION;
void flash_write_page(uint16_t start, uint8_t * page)
{
    cli();

    boot_page_erase(start);
    boot_spm_busy_wait();

    union {
        uint8_t bytes[2];
        uint16_t word;
    } u;

    uint8_t i;

    for(i=0; i<PAGE_SIZE; i+=2) {
        u.bytes[0] = *page++;
        u.bytes[1] = *page++;
        boot_page_fill(start + i, u.word);
        boot_spm_busy_wait();
    }

    boot_page_write(start);
    boot_spm_busy_wait();

    boot_rww_enable();

    sei();
}

void flash_write_bytes(uint16_t start, uint8_t * page, int num_bytes) BOOTLOADER_SECTION;
void flash_write_bytes(uint16_t start, uint8_t * page, int num_bytes)
{
    cli();

    union {
        uint8_t bytes[2];
        uint16_t word;
    } u;

    uint8_t i;
    uint8_t page_buf[PAGE_SIZE];
    uint16_t page_start;

    page_start = start & (~(PAGE_SIZE-1));

    printf("original page at %x:\n", page_start);
    for(i=0; i<PAGE_SIZE; i++) {
        page_buf[i] = pgm_read_byte(page_start + i);
        printf("%x ", page_buf[i]);
    }
    printf("\n");

    for(i=0; i < num_bytes; i++) {
        page_buf[start - page_start + i] = page[i];
        printf("%x ", page_buf[i]);
    }

    boot_page_erase(start);
    boot_spm_busy_wait();

    for(i=0; i < PAGE_SIZE; i+=2) {
        u.bytes[0] = page_buf[i];
        u.bytes[1] = page_buf[i+1];
        boot_page_fill(page_start + i, u.word);
        boot_spm_busy_wait();
    }

    boot_page_write(start);
    boot_rww_enable_safe();

    sei();
}

uint8_t char_to_hex(char c) {
    return isdigit(c) ? c - '0' : tolower(c) - 'a' + 0xA;
}

uint8_t str_to_hex(char *input, uint8_t* output) {
    int num_bytes = 0;
    while(*input) {
        *output++ = (char_to_hex(input[0])<<4) | char_to_hex(input[1]);
        input += 2;
        num_bytes++;
    }
    return num_bytes;
}

void flashed(void) __attribute__ ((section (FLASH_SECTION)));
void flashed() {
    LED_PORT |= LED_PORT; // dummy operation
}

int main(void)
{
    memset(instruction_buffer, 0xff, PAGE_SIZE);
    /* old LED test */
    memset(led_big_asm, 0xff, PAGE_SIZE);
    memset(led_small_asm, 0xff, PAGE_SIZE);

    led_big_asm[0] = 0x29;
    led_big_asm[1] = 0x9a;
    led_big_asm[2] = 0x08;
    led_big_asm[3] = 0x95;

    led_small_asm[0] = 0x28;
    led_small_asm[1] = 0x9a;
    led_small_asm[2] = 0x08;
    led_small_asm[3] = 0x95;

    LED_DDR |= (1<<PORTB0);
    LED_DDR |= (1<<PORTB1);

    LED_PORT = 0;

    _delay_ms(500);
    uart_init(BAUD(9600));
    printf("system start\n");

    int i;
    char buf[512];
    uint16_t flash_addr = 0x2000;

    /* sample commands
     * return: 08 95
     * turn on small LED (and return):  28 9a 08 95
     * turn on big LED (and return):    29 9a 08 95
     * turn off both LEDs (and return): 15 b8 08 95 */

    for(;;){
        printf("> ");
        fgets(buf,PAGE_SIZE,stdin);
        if (strlen(buf) && buf[strlen(buf)-1] == '\n'){
            buf[strlen(buf)-1] = '\0';

            // if the input starts with 'seek'
            if(strncmp(buf, "seek", strlen("seek")) == 0) {
                sscanf(buf + strlen("seek "), "%x", &flash_addr);
                printf("set flash_addr to %x\n", flash_addr);
            }
            else if(strcmp(buf, "run") == 0) {
                flashed();
            }
            else {
                int num_bytes = str_to_hex(buf, instruction_buffer);
                printf("prepping to flash %d bytes at address %x\n", num_bytes, flash_addr);
                flash_write_bytes(flash_addr, instruction_buffer, num_bytes);
                boot_rww_enable_safe();
                printf("after flashing: \n");
                uint16_t page_start = flash_addr & (~(PAGE_SIZE-1));
                for(i = 0; i < PAGE_SIZE; i++) {
                    printf("%x ", pgm_read_byte(page_start + i));
                }
                printf("\n");
                flash_addr += num_bytes;
                memset(buf, '\0', PAGE_SIZE);
                memset(instruction_buffer, 0xff, PAGE_SIZE);
            }
        }
    }
}
