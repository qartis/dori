#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "uart.h"

#define PAGE_SIZE 128
#define BUFFER_SIZE 512 + 1 // 512 + null byte
#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED_SMALL_BIT (1<<PORTB0)
#define LED_BIG_BIT (1<<PORTB1)

uint8_t led_small_asm[PAGE_SIZE];
uint8_t led_big_asm[PAGE_SIZE];

uint16_t str_len(const char *s) BOOTLOADER_SECTION;
uint16_t str_len(const char *s) {
    uint16_t n = 0;
    while(*s++) {
        n++;
    }

    return n;
}

uint16_t str_to_int(const char *s) BOOTLOADER_SECTION;
uint16_t str_to_int(const char *s) {
    uint8_t neg = 0;
    uint16_t len = str_len(s);
    uint16_t i;
    uint16_t num = 0;
    uint8_t mult = 10;

    if(s[0] == '-') {
        neg = 1;
        s++;
    }

    if(s[0] == '0' && s[1] == 'x') {
        mult = 16;
        s += 2;
    }

    for(i=0; i < len; i++) {
        int curr = s[i] - '0';

        if((curr >=0) && (curr <= 9))
        {
            num = num*mult + curr;
        }
        else { 
            //break on non-digit
            break;
        }
    }

    if(neg) {
        num = num*-1;
    }

    return num;
}

uint8_t str_equal(const char *s1, const char *s2) BOOTLOADER_SECTION;
uint8_t str_equal(const char *s1, const char *s2) {
    while(*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }

    return (!*s1 && !*s2);
}

uint8_t strn_equal(const char *s1, const char *s2, uint16_t n) BOOTLOADER_SECTION;
uint8_t strn_equal(const char *s1, const char *s2, uint16_t n) {
    uint16_t count = 0;
    while(*s1 && (*s1 == *s2)) {
        s1++;
        s2++;

        n++;
        if(n == count) {
            return 1;
        }
    }

    return (!*s1 && !*s2);
}



void flash_write(uint16_t start, uint8_t * bytes, int num_bytes) BOOTLOADER_SECTION;
void flash_write(uint16_t start, uint8_t * bytes, int num_bytes)
{
    uint8_t i;
    uint8_t page_buf[PAGE_SIZE];

    // if we received less bytes than the page size, then 
    if(num_bytes < PAGE_SIZE) {

        uint16_t page_start;
        page_start = start & (~(PAGE_SIZE-1));

        for(i=0; i<PAGE_SIZE; i++) {
            page_buf[i] = pgm_read_byte(page_start + i);
        }

        // page protection, don't go past page boundaries
        for(i=0; i < num_bytes && ((start - page_start + i) < PAGE_SIZE); i++) {
            page_buf[start - page_start + i] = bytes[i];
        }

        bytes = page_buf;
    }

    cli();

    boot_page_erase(start);
    boot_spm_busy_wait();

    union {
        uint8_t bytes[2];
        uint16_t word;
    } u ;

    for(i=0; i<PAGE_SIZE; i+=2) {
        u.bytes[0] = *bytes++;
        u.bytes[1] = *bytes++;
        boot_page_fill(start + i, u.word);
        boot_spm_busy_wait();
    }

    boot_page_write(start);
    boot_spm_busy_wait();

    boot_rww_enable_safe();

    sei();
}

uint8_t char_to_hex(char c) BOOTLOADER_SECTION;
uint8_t char_to_hex(char c) {
    if(c >= '0' && c <= '9') {
        return c - '0';
    }
    else {
        return (c | 0x20) - 'a' + 0xA;
    }
}

uint8_t str_to_hex(char *input, uint8_t* output) BOOTLOADER_SECTION;
uint8_t str_to_hex(char *input, uint8_t* output) {
    int num_bytes = 0;
    while(*input && num_bytes < PAGE_SIZE) {
        *output++ = (char_to_hex(input[0])<<4) | char_to_hex(input[1]);
        input += 2;
        num_bytes++;
    }
    return num_bytes;
}

void flashed(void) __attribute__ ((section (FLASH_SECTION)));
void flashed() {
    PORTB |= (1<<PORTB1);
}

void bootloader(void) BOOTLOADER_SECTION;
void bootloader(void) {
    uint8_t instruction_buffer[PAGE_SIZE];

    // init LEDs
    LED_DDR |= (1<<PORTB0);
    LED_DDR |= (1<<PORTB1);
    LED_PORT = 0;

    _delay_ms(500);
    uart_init(BAUD(9600));

    char buf[BUFFER_SIZE + 1];
    uint16_t flash_addr = FLASH_SECTION_ADDR;
    unsigned char flash_mode = 0;

    /* sample commands
     * return: 08 95
     * turn on small LED (and return):  28 9a 08 95
     * turn on big LED (and return):    29 9a 08 95
     * turn off both LEDs (and return): 15 b8 08 95 */

    uint16_t rc = 0;

    for(;;) {
        buf[rc++] = uart_getchar();
        if(buf[rc-1] == '\n' || rc == PAGE_SIZE*2 ) {
            if(buf[rc-1] == '\n') {
                buf[rc-1] = '\0';
            }
            else {
                buf[rc] = '\0';
            }

            rc = 0;

            uart_print(buf);
            uart_putchar('\n');

            if(flash_mode) {
                if(strn_equal(buf, "===done===", str_len("===done==="))) {
                    uart_print("out of flash mode\n");
                    flash_mode = 0;
                }
                else {
                    char *buf_ptr = buf;

                    while(*buf_ptr) {
                        uint16_t num_bytes = str_to_hex(buf_ptr, instruction_buffer);
                        buf_ptr += num_bytes * 2;
                        flash_write(flash_addr, instruction_buffer, num_bytes);
                        boot_rww_enable_safe();
                        flash_addr += num_bytes;
                        uart_print("just flashed to address ");
                        uart_printint(flash_addr);
                        uart_putchar('\n');
                    }
                }
            }
            else {
                if(str_equal(buf, "run")) {
                    flashed();
                }
                else if(str_equal(buf, "flash")) {
                    flash_mode = 1;
                    uart_print("in flash mode\n");
                }
                else if(strn_equal(buf, "seek", str_len("seek")) == 0) {
                    flash_addr = str_to_int(buf + str_len("seek "));
                    uart_print("\nseeking to:");
                    uart_printint(flash_addr);
                    uart_putchar('\n');
                }
            }
        }
    }
}

int main(void)
{
    bootloader();
}
