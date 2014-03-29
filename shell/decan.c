#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "../can.h"
#include "../can_names.h"

#define RED "\e[1;31m"
#define GREEN "\e[1;32m"
#define YELLOW "\e[1;33m"
#define BLUE "\e[1;34m"
#define COLOR "\e[1;35m"
#define CYAN "\e[1;36m"
#define NC "\e[0m"

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

char *getcolorbytype(int type)
{
    switch (type) {
    case TYPE_value_request:
        return YELLOW;
    case TYPE_value_periodic:
        return GREEN;
    case TYPE_value_explicit:
        return BLUE;
    case TYPE_sensor_error:
        return RED;
    case TYPE_action_modema_powercycle ... TYPE_action_drive:
        return RED;
    case TYPE_sos_reboot ... TYPE_sos_nostfu:
        return RED;
    default:
        return "";
    }
}

void decan(int type, int id, int sensor, int len, uint8_t * data)
{
    int i;

    printf("Frame: %s%s" NC "[%02x] %s[%02x].%s[%02x] ", getcolorbytype(type),
           type_names[type], type, id_names[id], id, sensor_names[sensor],
           sensor);

    if (type == TYPE_value_explicit && id == ID_enviro && sensor ==
        SENSOR_pressure) {
        int pressure =
            data[0] << 24 |
            data[1] << 16 |
            data[2] << 8 |
            data[3] << 0;

        printf("%u Pa\n", pressure);
    } else if (type == TYPE_value_explicit && id == ID_diag && sensor ==
               SENSOR_voltage) {
        int num = (data[0] << 8) | data[1];
        data[7] = '\0';
        printf("0x%02x (%sV)\n", num, data + 2);
    } else if (type == TYPE_value_explicit && id == ID_diag && sensor ==
               SENSOR_current) {
        int num = (data[0] << 8) | data[1];
        printf("%umA\n", num);
    } else if ((type == TYPE_value_explicit || type == TYPE_value_periodic) &&
               sensor >= SENSOR_temp0 && sensor <= SENSOR_temp8) {
        int temp = (data[0] << 8) | data[1];
        printf("%u.%u°C\n", temp / 10, temp % 10);
    } else if ((type == TYPE_value_explicit || type == TYPE_value_periodic) &&
               sensor == SENSOR_time) {
        time_t time =
            (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3] << 0);
        printf("%s", ctime(&time));
    } else if ((type == TYPE_value_explicit || type == TYPE_value_periodic) &&
               sensor == SENSOR_laser) {
        uint16_t dist = (data[0] << 8) | data[1];
        printf("%u.%um\n", dist / 1000, dist % 1000);
    } else if ((type == TYPE_value_explicit || type == TYPE_value_periodic) &&
               sensor == SENSOR_coords) {

        int32_t lat =
            (data[0] << 24) |
            (data[1] << 16) |
            (data[2] << 8) |
            (data[3] << 0);

        int32_t lon = (data[4] << 24) |
            (data[5] << 16) | (data[6] << 8) | (data[7] << 0);

        float lat_f = ((lat) / 10000000) + ((lat) % 10000000) / 6000000.0;
        float lon_f = ((lon) / 10000000) + ((lon) % 10000000) / 6000000.0;
        printf(RED "https://maps.google.ca/maps?output=classic&q=%f,%f\n" NC,
               lat_f, lon_f);
    } else if ((type == TYPE_value_explicit || type == TYPE_value_periodic) &&
               (sensor == SENSOR_gyro || sensor == SENSOR_compass)) {
        int16_t x = (data[0] << 8) | data[1];
        int16_t y = (data[2] << 8) | data[3];
        int16_t z = (data[4] << 8) | data[5];

        printf("%+d %+d %+d\n", x, y, z);
    } else {
        if (len > 0)
            printf("%d:", len);

        for (i = 0; i < len; i++) {
            printf(" %02x", data[i]);
        }

        printf("\n");
    }

    printf(NC);
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

        if (type == TYPE_nop)
            continue;

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
