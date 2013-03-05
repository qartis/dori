#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

#include "laser.h"

void laser_init(void)
{
    LASER_DDR |= (1 << LASER_PWR);
    LASER_DDR &= ~((1 << LASER_EOF) | (1 << LASER_SCK) | (1 << LASER_MOSI));
}

void laser_on(void)
{
    LASER_PORT |= (1 << LASER_PWR);
    _delay_ms(100);
    LASER_PORT &= ~(1 << LASER_PWR);
}

uint16_t decode(uint8_t *byte)
{
    uint16_t num = 0;
    int8_t i;

    for (i = 4; i >= 0; i--) {
        uint8_t mix = (byte[i*2] & 0b01110111) << 1
                    | (byte[i*2+1] >> 7);

        num *= 10;

        switch (mix) {
        case 0xaf: num += 0; break;
        case 0x00: num += 0; break;
        case 0x06: num += 1; break;
        case 0x6d: num += 2; break;
        case 0x4f: num += 3; break;
        case 0xc6: num += 4; break;
        case 0xcb: num += 5; break;
        case 0xeb: num += 6; break;
        case 0x0e: num += 7; break;
        case 0xef: num += 8; break;
        case 0xcf: num += 9; break;
        case 0x40: /* '-' */ 
        case 0xe9: /* 'E' */
        case 0x60: /* 'r' */
        default: return LASER_ERROR;
        }
    }

    if (num == 2080) {
        num = LASER_ERROR;
    }

    return num;
}

uint16_t laser_read(void)
{
    uint8_t i;
    uint16_t j;
    uint8_t bits[58 * 8];
    uint8_t bytes[58];
    uint16_t retry;

    laser_on();
    _delay_ms(200);
    laser_on();
    _delay_ms(1300);

    retry = 0xffff;

    while (!(LASER_PIN & (1 << LASER_EOF)) && retry) { retry--; };
    while (LASER_PIN & (1 << LASER_EOF) && retry) { retry--; };

    if (retry == 0) {
        return LASER_ERROR;
    }

    for(j=1;j<58 * 8;){
        while (LASER_PIN & (1 << LASER_SCK) && retry) { retry--; };
        while (!(LASER_PIN & (1 << LASER_SCK)) && retry) { retry--; };
        bits[j++] = LASER_PIN;
        while (LASER_PIN & (1 << LASER_SCK) && retry) { retry--; };
        while (!(LASER_PIN & (1 << LASER_SCK)) && retry) { retry--; };
        bits[j++] = LASER_PIN;
        while (LASER_PIN & (1 << LASER_SCK) && retry) { retry--; };
        while (!(LASER_PIN & (1 << LASER_SCK)) && retry) { retry--; };
        bits[j++] = LASER_PIN;
        while (LASER_PIN & (1 << LASER_SCK) && retry) { retry--; };
        while (!(LASER_PIN & (1 << LASER_SCK)) && retry) { retry--; };
        bits[j++] = LASER_PIN;
        while (LASER_PIN & (1 << LASER_SCK) && retry) { retry--; };
        while (!(LASER_PIN & (1 << LASER_SCK)) && retry) { retry--; };
        bits[j++] = LASER_PIN;
        while (LASER_PIN & (1 << LASER_SCK) && retry) { retry--; };
        while (!(LASER_PIN & (1 << LASER_SCK)) && retry) { retry--; };
        bits[j++] = LASER_PIN;
        while (LASER_PIN & (1 << LASER_SCK) && retry) { retry--; };
        while (!(LASER_PIN & (1 << LASER_SCK)) && retry) { retry--; };
        bits[j++] = LASER_PIN;
        while (LASER_PIN & (1 << LASER_SCK) && retry) { retry--; };
        while (!(LASER_PIN & (1 << LASER_SCK)) && retry) { retry--; };
        bits[j++] = LASER_PIN;
    }

    if (retry == 0) {
        return LASER_ERROR;
    }

    for(j=0;j<58;j++){
        bytes[j] = 0;
        for(i=0;i<8;i++){
            bytes[j] <<= 1;
            if (bits[j * 8 + i] & (1 << LASER_MOSI)) {
                bytes[j] |= 1;
            }
        }
    }

    return decode(bytes + 40);
}
