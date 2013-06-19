#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>

#include "ts555.h"
#include "timer.h"

volatile uint8_t ts555_active;

inline void ts555_high(void){
    PORTB |= (1<<PORTB2);
}

inline void ts555_low(void){
    PORTB &= ~(1<<PORTB2);
}

void ts555_reset(void){
    DDRB |= (1<<PORTB3);
    PORTB &= ~(1<<PORTB3);
    DDRB &= ~(1<<PORTB3);
}

void ts555_deactivate(void){
    ts555_active = 0;
}

void ts555_trigger(void){
    ts555_active = 0;
    ts555_reset();
    delay_ms(1);

    ts555_low();
    ts555_high();
    delay_ms(1);
    ts555_active = 1;
}

void ts555_init(void){
    ts555_active = 0;
    DDRB |= (1<<PORTB2);
    DDRB &= ~(1<<PORTB1);
    ts555_high();
    PCMSK0 |= 1<<PCINT1;
    PCICR |=  1<<PCIE0;
    ts555_reset();
}
