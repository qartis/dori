#define ON_BTN  DDRD, PORTD, PIND6
#define OFF_BTN DDRD, PORTD, PIND3

#define TAP_OFF() do { \
     DDRD |= (1 << PORTD3); \
     PORTD |= (1 << PORTD3); \
     _delay_ms(100); \
     PORTD &= ~(1 << PORTD3); \
     DDRD &= ~(1 << PORTD3); \
     _delay_ms(100); } while(0);


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
     _delay_ms(500); \
     ddr &= ~(1 << pin); \
     port &= ~(1 << pin); \
    })


#define \
    PRESS(X) \
    press(X)

#define \
    press(ddr, port, pin) \
    ({ \
     ddr |= (1 << pin); \
     port |= (1 << pin); \
     _delay_ms(250); \
     ddr &= ~(1 << pin); \
     port &= ~(1 << pin); \
     _delay_ms(1000); \
    })

