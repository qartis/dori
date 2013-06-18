#include <stdint.h>
#include <avr/io.h>

#include "led.h"


void led_init(void){
    LED_DDR |= LED_BIT;
    led_off();
}

void led_off(void){
    LED_PORT &= ~(LED_BIT);
}

void led_on(void){
    LED_PORT |= LED_BIT;
}

void led_toggle(void){
    LED_PORT ^= LED_BIT;
}

uint8_t led_status(void){
    return !(PINB & LED_BIT);
}
