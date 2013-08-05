#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../can.h"
#include "../can_names.h"

char readbyte(void)
{
    int c;

    c = getchar();

    if (c == EOF) {
        printf("done\n");
        exit(0);
    }
    
    return c;
}

void decan(int type, int id, int len, uint8_t *data)
{
    int i;

    printf("Frame: %s [%x] %s [%x] %d", type_names[type], type,
        id_names[id], id, len);

    if (len > 0)
        printf(":");

    for (i = 0; i < len; i++) {
        printf(" %x", data[i]);
    }

    printf("\n");
}

int main(void)
{
    int i;
    int type;
    int id;
    int len;
    uint8_t data[8];

    for (;;) {
        type = readbyte();
        id = readbyte();
        len = readbyte();

        if (len > 8) {
            printf("len > 8\n");
            exit(1);
        }

        for (i = 0; i < len; i++)
            data[i] = readbyte();

        decan(type, id, len, data);
    }

    return 0;
}
