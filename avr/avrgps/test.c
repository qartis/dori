#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char **argv){
    int h, m, s;
    int mon, d, y;

    scanf("%d %d %d %d %d %d", &h, &m, &s, &d, &mon, &y);
    struct tm tm;
    tm.tm_sec = s;
    tm.tm_min = m;
    tm.tm_hour = h;
    tm.tm_mday = d;
    tm.tm_mon = mon - 1;
    tm.tm_year = y - 1900;

    tm.tm_isdst = 0;

    setenv("TZ", "UTC", 1);
    time_t time = mktime(&tm);

    printf("time: %lu\n", time);
}

