#define ON_BTN  DDRD, PORTD, PIND5
#define OFF_BTN DDRD, PORTD, PIND6

#define VPLUS_PIN PIND
#define VPLUS_BIT PIND7

#define \
    HOLD(X) \
    hold(X)

#define \
    hold(ddr, port, pin) \
    ({ \
     ddr |= (1 << pin); \
     port |= (1 << pin); \
     _delay_ms(2000); \
     ddr &= ~(1 << pin); \
     port &= ~(1 << pin); \
     _delay_ms(1000); \
    })


#define \
    PRESS(X) \
    press(X)

#define \
    press(ddr, port, pin) \
    ({ \
     ddr |= (1 << pin); \
     port |= (1 << pin); \
     _delay_ms(500); \
     ddr &= ~(1 << pin); \
     port &= ~(1 << pin); \
     _delay_ms(1000); \
    })

