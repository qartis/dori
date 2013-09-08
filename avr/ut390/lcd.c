#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>

void toggle_en(void)
{
    PORTD &= ~(1 << PORTD4);
    _delay_ms(1);
    PORTD |= (1 << PORTD4);
}

void write_string(const char *str)
{
    PORTD |= (1 << PORTD3);

    while (*str) {
        PORTC = (*str & 0xf0) >> 4;
        toggle_en();
        PORTC = (*str & 0x0f);
        toggle_en();
        _delay_ms(2);
        str++;
    }
}

void lcd_init(void)
{
    DDRD = (1 << PORTD3) | (1 << PORTD4);
    DDRC = 0xff;
    PORTD = 0;
    PORTC = 0;

    PORTD |= (1 << PORTD4);

    _delay_ms(500);

    PORTC = 0x03;
    toggle_en();
    _delay_ms(5);

    toggle_en();
    _delay_ms(5);

    toggle_en();
    _delay_ms(5);

    PORTC = 0x02;
    toggle_en();
    _delay_ms(5);




    /* 2 line display */
    PORTC = 0x02;
    toggle_en();
    _delay_ms(5);

    PORTC = 0x08;
    toggle_en();
    _delay_ms(5);




    /* turn off the display */
    PORTC = 0x00;
    toggle_en();
    _delay_ms(5);

    PORTC = 0x08;
    toggle_en();
    _delay_ms(5);






    /* clear display ram */
    PORTC = 0x00;
    toggle_en();
    _delay_ms(5);

    PORTC = 0x01;
    toggle_en();
    _delay_ms(5);





    /* enable cursor move dir */
    PORTC = 0x00;
    toggle_en();
    _delay_ms(5);

    PORTC = 0x06;
    toggle_en();
    _delay_ms(5);



    /* turn lcd back on */
    PORTC = 0x00;
    toggle_en();
    _delay_ms(5);

    PORTC = 0x0C;
    toggle_en();
    _delay_ms(5);
}
