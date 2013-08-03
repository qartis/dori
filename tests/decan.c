#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../can.h"

#define X(name, value) char temp_type_ ## name [] = #name;
TYPE_LIST(X)
#undef X

#define X(name, value) char temp_id_ ## name [] = #name;
ID_LIST(X)
#undef X


char unknown_string [] = "???";

char *type_names [] = {
    [0 ... 128] = unknown_string,
#define X(name, value) [value] = temp_type_ ##name,
    TYPE_LIST(X)
#undef X
};


char *id_names [] = {
    [0 ... 64] = unknown_string,
#define X(name, value) [value] = temp_id_ ##name,
    ID_LIST(X)
#undef X
};


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
