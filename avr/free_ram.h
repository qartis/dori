/* #include <avr/pgmspace.h> */

uint16_t free_ram(void){
    extern unsigned int *__brkval, __heap_start;

    return SP - (__brkval ? (unsigned int)__brkval : (unsigned int)&__heap_start);
}
