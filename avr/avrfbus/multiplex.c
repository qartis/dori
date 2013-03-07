#include <avr/io.h>

#include "multiplex.h"

void multiplex_init(void){
    DDRC |= (1<<PORTC5) | (1<<PORTC4);
    multiplex_phone();
}

void multiplex_gps(void){
    PORTC |= (1<<PORTC4);
    PORTC &= ~(1<<PORTC5);
}

void multiplex_phone(void){
    PORTC |= (1<<PORTC5);
    PORTC &= ~(1<<PORTC4);
}
