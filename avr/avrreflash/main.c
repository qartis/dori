#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uart.h"

#define PAGE_SIZE 128
#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED_SMALL_BIT (1<<PORTB0)
#define LED_BIG_BIT (1<<PORTB1)

uint8_t led_big_asm[PAGE_SIZE];
uint8_t led_small_asm[PAGE_SIZE];

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

void led_on(void) __attribute__ ((section (".led")));
void led_on() {
    LED_PORT |= LED_PORT;
}

int main(void)
{
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

    char buf[128];

    for(;;){
        printf("> ");
        fgets(buf,128,stdin);
        if (strlen(buf) && buf[strlen(buf)-1] == '\n'){
            buf[strlen(buf)-1] = '\0';
            if(strcmp(buf, "big") == 0) {
                LED_PORT = 0;
                flash_write(0x1000, led_big_asm);
                led_on();
            }
            else if(strcmp(buf, "small") == 0) {
                LED_PORT = 0;
                flash_write(0x1000, led_small_asm);
                led_on();
            }
        }
    }
}
