#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../can.h"
#include "../can_names.h"

unsigned char readbyte(void)
{
    int c;

    c = getchar();

    if (c == EOF) {
        printf("done\n");
        exit(0);
    }

    return c;
}

void decan(int type, int id, int sensor, int len, uint8_t *data)
{
    int i;

    printf("Frame: %s[%x] %s[%x].%s[%x] ", type_names[type], type,
        id_names[id], id, sensor_names[sensor], sensor);

    if (type == TYPE_value_explicit && id == ID_enviro && sensor ==
            SENSOR_pressure) {
        int pressure = data[0] << 24 |
                       data[1] << 16 |
                       data[2] << 8  |
                       data[3] << 0;

        printf("%u pascals\n", pressure);

        return;
    } else if (type == TYPE_value_explicit && id == ID_diag && sensor ==
            SENSOR_voltage) {
        int num = (data[0] << 8) | data[1];
        data[7] = '\0';
        printf("0x%02x (%sV)\n", num, data + 2);

        return;
    }

    if (len > 0)
        printf("%d:", len);

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
    int sensor;
    uint8_t data[8];

    for (;;) {
        type = readbyte();

        if (type == TYPE_nop) continue;

        id = readbyte();
        sensor = readbyte() << 8;
        sensor |= readbyte();
        len = readbyte();

        if (len > 8) {
            printf("len > 8\n");
            exit(1);
        }

        for (i = 0; i < len; i++)
            data[i] = readbyte();

        decan(type, id, sensor, len, data);
    }

    return 0;
}
