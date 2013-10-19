#define HI(arg) hi(arg)
#define hi(port, pin) \
    DDR ## port |= (1 << pin); \
    PORT ## port |= (1 << pin);

#define LOW(arg) low(arg)
#define low(port, pin) \
    DDR ## port |= (1 << pin); \
    PORT ## port &= ~(1 << pin);

#define HIZ(arg) hiz(arg)
#define hiz(port, pin) \
    DDR ## port &= ~(1 << pin); \
    PORT ## port &= ~(1 << pin);


#define CS    D, 7
#define POWER D, 6
#define SNAP D, 5
#define RELAY D, 4

void mode_auto(void)
{
    /*
    HIZ(MODE1);
    HIZ(MODE2);
    HIZ(MODE3);
    HIZ(MODE4);
    HIZ(MODE5);
    HIZ(MODE6);
    */
}

