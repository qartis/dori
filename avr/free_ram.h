/* #include <avr/pgmspace.h> */

uint16_t free_ram(void)
{
    extern unsigned int *__brkval, __heap_start;

    return SP - (__brkval ? (unsigned int)__brkval : (unsigned int)&__heap_start);
}

extern uint8_t _end;
extern uint8_t __stack;

#define STACK_CANARY  0x55

void stack_paint(void) __attribute__ ((naked)) __attribute__ ((section (".init1")));

void stack_paint(void)
{
    uint8_t *p;

    for (p = &_end; p <= &__stack; p++) {
        *p = STACK_CANARY;
    }
}

uint16_t stack_space(void)
{
    uint8_t *p = &_end;
    uint16_t c = 0;

    while (*p == STACK_CANARY && p <= &__stack) {    
        p++;
        c++;
    }

    return c;
}
