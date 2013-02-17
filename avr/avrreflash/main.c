#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/sleep.h>
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

void flash_write(uint16_t start, uint8_t * page) BOOTLOADER_SECTION;
void flash_write(uint16_t start, uint8_t * page)
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

int char_to_hex(char c) {
    return isdigit(c) ? c - '0' : tolower(c) - 'a' + 0xA;
}

void str_to_hex(char *input, uint8_t* output) {
    while(*input) {
        *output++ = (char_to_hex(input[0])<<4) | char_to_hex(input[1]);
        input += 2;
    }
}

void flashed(void) __attribute__ ((section (FLASH_SECTION)));
void flashed() {
    LED_PORT |= LED_PORT; // dummy operation
}

int main(void)
{
    memset(instruction_buffer, 0xff, PAGE_SIZE);
    /* old LED test
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
    */

    LED_DDR |= (1<<PORTB0);
    LED_DDR |= (1<<PORTB1);

    LED_PORT = 0;

    _delay_ms(500);
    uart_init(BAUD(9600));
    printf("system start\n");

    char buf[512];

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
            str_to_hex(buf, instruction_buffer);
            flash_write(0x1000, instruction_buffer);
            flashed();
            memset(buf, '\0', PAGE_SIZE);
            memset(instruction_buffer, 0xff, PAGE_SIZE);
        }
    }
}
