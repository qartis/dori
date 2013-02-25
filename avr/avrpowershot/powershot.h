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

#define MODE1 B, 2
#define MODE2 D, 5
#define MODE3 B, 0
#define MODE4 D, 6
#define MODE5 B, 1
#define MODE6 D, 4

#define SNAP B, 3
#define FOCUS D, 3
#define FUNC C, 0
#define DISP D, 7
#define MENU C, 4
#define UP C, 2
#define DOWN C, 1
#define LEFT C, 5
#define RIGHT C, 3
#define ZOOMIN C, 6
#define ZOOMOUT C, 7

#define POWER D, 2

void mode_auto(void)
{
    HIZ(MODE1);
    HIZ(MODE2);
    HIZ(MODE3);
    HIZ(MODE4);
    HIZ(MODE5);
    HIZ(MODE6);
}

