#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart.h"
#include "lcd.h"

char *getline(char *s, int size)
{
    char *p;
    int c;

    p = s;

    while (p < s + size - 1){
        c = getchar();
        
        if (c == '\r')
            continue;

        *p++ = c;

        if (c == '\n')
            break;
    }
    *p = '\0';

    return s;
}

int strstart_P(const char *s1, const char * PROGMEM s2)
{
    return strncmp_P(s1, s2, strlen_P(s2)) == 0;
}

int getdist(void)
{
    char buf[64];
    char *comma;
    int dist;

    for (;;) {
        getline(buf, sizeof(buf));
        if (!strstart_P(buf, PSTR("Dist: ")))
            continue;

        comma = strchr(buf, ',');
        if (comma == NULL)
            continue;

        *comma = '\0';

        dist = atoi(buf + strlen_P(PSTR("Dist: ")));
        return dist;
    }
}

void main(void)
{
    _delay_ms(300);
    uart_init(BAUD(115200));
    lcd_init();
    sei();
    int dist_mm = 0;
    int dist_m = 0;
    char buf[128];

    for (;;) {
//        dist_mm = getdist();
        printf("dist: %d\n", dist_mm);
        dist_m = dist_mm / 1000;
        snprintf_P(buf, sizeof(buf),
                PSTR("Dist: %d.%dm "),
                dist_m, dist_mm % 1000);
        //write_string("Dist: 32.930m");
        _delay_ms(500);
    }

}
