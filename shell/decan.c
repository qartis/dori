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

void print_time_chunk(int val, const char *unit)
{
    if (val > 0) {
        printf("%d %s%s ", val, unit, val != 1 ? "s" : "");
    }
}

void print_time(int seconds)
{
    int days = seconds / (60 * 60 * 24);
    int hours = (seconds / (60 * 60)) % 24;
    int minutes = (seconds / 60) % 60;

    if (seconds == 0) {
        printf("0 seconds");
    }

    seconds = seconds % 60;

    print_time_chunk(days, "day");
    print_time_chunk(hours, "hour");
    print_time_chunk(minutes, "minute");
    print_time_chunk(seconds, "second");

    printf("\n");
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

int print_value(int sensor, uint8_t *data)
{
    int intval;
    time_t timeval;

    switch (sensor) {
    case SENSOR_pressure:
        intval =
            data[0] << 24 |
            data[1] << 16 |
            data[2] << 8 |
            data[3] << 0;

        printf("%u Pa\n", intval);
        break;
    case SENSOR_voltage:
        intval = (data[0] << 8) | data[1];
        double slope = 0.00509924;
        double voltage = slope * intval;
        voltage += 10.0776;
        printf("0x%02x (%fV)\n", intval, voltage);
        break;
    case SENSOR_sats:
        printf("%d satellites\n", data[0]);
        break;
    case SENSOR_current:
        intval = (data[0] << 8) | data[1];
        printf("%umA\n", intval);
        break;
    case SENSOR_temp0 ... SENSOR_temp8:
        intval = (data[0] << 8) | data[1];
        printf("%u.%u\xc2\xb0""C\n", intval / 10, intval % 10);
        break;
    case SENSOR_time:
        timeval =
            (data[0] << 24) |
            (data[1] << 16) |
            (data[2] << 8)  |
            (data[3] << 0);
        printf("%s", ctime(&timeval));
        break;
    case SENSOR_laser:
        intval = (data[0] << 8) | data[1];
        printf("%u.%um\n", intval / 1000, intval % 1000);
        break;
    case SENSOR_coords:
        {
            int32_t lat =
                (data[0] << 24) |
                (data[1] << 16) |
                (data[2] << 8) |
                (data[3] << 0);

            int32_t lon = (data[4] << 24) |
                (data[5] << 16) | (data[6] << 8) | (data[7] << 0);

            float lat_f = ((lat) / 10000000) + ((lat) % 10000000) / 6000000.0;
            float lon_f = ((lon) / 10000000) + ((lon) % 10000000) / 6000000.0;
            printf(RED "https://maps.google.ca/maps?output=classic&q=%f,%f\n",
                lat_f, lon_f);
            break;
        }
    case SENSOR_gyro:
    case SENSOR_compass:
    case SENSOR_accel:
        {
            int16_t x = (data[0] << 8) | data[1];
            int16_t y = (data[2] << 8) | data[3];
            int16_t z = (data[4] << 8) | data[5];

            printf("%+d %+d %+d\n", x, y, z);
            break;
        }
    case SENSOR_uptime:
        intval =
            (data[0] << 24) |
            (data[1] << 16) |
            (data[2] << 8)  |
            (data[3] << 0);
        print_time(intval);
        break;
    case SENSOR_boot:
#define PORF 0
#define EXTRF 1
#define BORF 2
#define WDRF 3
        intval = data[0];
        printf((intval & (1 << WDRF)) ? "wd" : "");
        printf((intval & (1 << BORF)) ? "bo" : "");
        printf((intval & (1 << EXTRF)) ? "ext" : "");
        printf((intval & (1 << PORF)) ? "po" : "");
        printf((intval & ((1 << PORF) | (1 << EXTRF) | (1 << BORF) | (1 << WDRF))) == 0 ? "soft    ware" : "");
        printf(" reboot\n"); 
        break;
    case SENSOR_interval:
        intval = data[0] << 8 | data[1];
        printf("%d seconds\n", intval);
        break;
    default:
        return 1;
    }

    return 0;
}

const char *ptime(void)
{
    time_t t;
    struct tm *tm;
    static char buf[64];

    time(&t);
    tm = localtime(&t);
    strftime(buf, sizeof(buf), "%c", tm);

    return buf;
}

void decan(int type, int id, int sensor, int len, uint8_t * data)
{
    int i;
    int rc;

    printf("[%s] %s%s" NC "[%02x] %s[%02x].%s[%02x] ", ptime(),
            getcolorbytype(type), type_names[type], type, id_names[id],
            id, sensor_names[sensor], sensor);

    rc = 1;

    if (type == TYPE_value_explicit || type == TYPE_value_periodic) {
        rc = print_value(sensor, data);
    }

    if (rc) {
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
