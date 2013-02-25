#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "nmea.h"

#define BUFLEN 80+2+1 /* 80 lines of text, plus possible \r\n, plus NULL */

int main(void) {
    char buf[BUFLEN];
    unsigned int buflen = 0;

    printf("system start\n");

    for(;;){
        if (buflen > sizeof(buf)){
            //wtf, nmea shouldn't be this long. drop it
            buflen = 0;
            continue;
        }
        char c = getchar();
        if (!isprint(c) && c != '\n'){
            continue;
        }
        if (c == '\n'){
            if (buflen > 0){
                buf[buflen] = '\0';
                parse_nmea(buf);
            }
            buflen = 0;
            continue;
        }
        buf[buflen++] = c;
    }
}
