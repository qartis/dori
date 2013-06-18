#define IR_GND_DDR DDRC
#define IR_GND_PIN PINC
#define IR_GND1 PORTC5
#define IR_GND2 PORTC4
#define IR_GND3 PORTC3
#define IR_GND4 PORTC2

#define IR_DATA5 PORTC0
#define IR_DATA7 PORTC1

#if 0
    uint8_t state[4];
    uint8_t state_inv[4];
    uint8_t state2[4];

//#define DELAY _delay_us(0000);
#define DELAY _delay_us(30);

    uart_init(4);

    for(;;){
        printf("%d", !!(IR_GND_PIN & 0b00100000));
        printf("%d", !!(IR_GND_PIN & 0b00010000));
        printf("%d", !!(IR_GND_PIN & 0b00001000));
        printf("%d ",!!(IR_GND_PIN & 0b00000100));
        printf("%x", (PIND >> 2) & 0b000011);
        putchar('\n');
    }

    for (;;) {
        while ((IR_GND_PIN & (1 << IR_GND1))) { };
        while (!(IR_GND_PIN & (1 << IR_GND1))) { };
        DELAY;
        state[0] = IR_GND_PIN;
        state2[0] = PIND;


        while ((IR_GND_PIN & (1 << IR_GND2))) { };
        while (!(IR_GND_PIN & (1 << IR_GND2))) { };
        DELAY;
        state[1] = IR_GND_PIN;
        state2[1] = PIND;

        while ((IR_GND_PIN & (1 << IR_GND3))) { };
        while (!(IR_GND_PIN & (1 << IR_GND3))) { };
        DELAY;
        state[2] = IR_GND_PIN;
        state2[2] = PIND;

        while ((IR_GND_PIN & (1 << IR_GND4))) { };
        while (!(IR_GND_PIN & (1 << IR_GND4))) { };
        DELAY;
        state[3] = IR_GND_PIN;
        state2[3] = PIND;

        printf(" 8_4:  %c \n", state2[3] & (1 << PORTD3) ? ' ' : '_');
        printf("      %c",     state2[3] & (1 << PORTD2) ? ' ' : '|');
        printf("%c",           state2[2] & (1 << PORTD2) ? ' ' : '_');
        printf("%c\n",         state2[2] & (1 << PORTD3) ? ' ' : '|');
        printf("      %c",     state2[1] & (1 << PORTD2) ? ' ' : '|');
        printf("%c",           state2[0] & (1 << PORTD3) ? ' ' : '_');
        printf("%c\n",         state2[1] & (1 << PORTD3) ? ' ' : '|');

        printf(" 8_5:  %c \n", state[3] & (1 << IR_DATA7) ? ' ' : '_');
        printf("      %c",     state[3] & (1 << IR_DATA5) ? ' ' : '|');
        printf("%c",           state[2] & (1 << IR_DATA5) ? ' ' : '_');
        printf("%c\n",         state[2] & (1 << IR_DATA7) ? ' ' : '|');
        printf("      %c",     state[1] & (1 << IR_DATA5) ? ' ' : '|');
        printf("%c",           state[0] & (1 << IR_DATA7) ? ' ' : '_');
        printf("%c\n\n",       state[1] & (1 << IR_DATA7) ? ' ' : '|');
        _delay_ms(500);
    }
}
#endif
