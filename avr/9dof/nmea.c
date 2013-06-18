#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <avr/pgmspace.h>

struct tm {
    uint8_t tm_sec;     /* seconds (0 - 60) */
    uint8_t tm_min;     /* minutes (0 - 59) */
    uint8_t tm_hour;    /* hours (0 - 23) */
    uint8_t tm_mday;    /* day of month (1 - 31) */
    uint8_t tm_mon;     /* month of year (0 - 11) */
    uint8_t tm_year;    /* year - 1900 */
};

uint8_t num_satellites;
uint32_t cur_time;
int32_t cur_lat;
int32_t cur_lon;

uint32_t mktime(struct tm *t)
{
    uint16_t month, year;
    uint32_t result;
    uint16_t m_to_d[] =
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    month = t->tm_mon;
    year = t->tm_year + month / 12 + 1900;
    month %= 12;
    result = (year - 1970) * 365 + m_to_d[month];

    if (month <= 1)
        year -= 1;

    result += (year - 1968) / 4;
    result -= (year - 1900) / 100;
    result += (year - 1600) / 400;
    result += t->tm_mday;
    result -= 1;
    result *= 24;
    result += t->tm_hour;
    result *= 60;
    result += t->tm_min;
    result *= 60;
    result += t->tm_sec;
    return result;
}

uint8_t strstart(const char *a, const char *b)
{
    return strncmp(a, b, strlen(b)) == 0;
}

uint8_t parsedec2(const char *buf)
{
    return (buf[0]-'0')*10 + (buf[1]-'0');
}

uint32_t parsedecn(const char *buf, uint8_t i)
{
    uint32_t val = 0;

    while (i--) {
        val *= 10;
        val += *buf - '0';
        buf++;
    }

    return val;
}

uint8_t from_hex(char a)
{
    if (isupper(a)) {
        return a - 'A' + 10;
    } else if (islower(a)) {
        return a - 'a' + 10;
    } else {
        return a - '0';
    }
}

uint16_t checksum(const char *buf)
{
    uint8_t device_cksum;
    uint8_t calc_cksum = 0;

    buf++; //skip the $

    while (*buf != '\0' && *buf != '*') {
        calc_cksum ^= *buf++;
    }
    if (*buf != '*') return 0;

    buf++; //skip the *

    device_cksum = 16 * from_hex(buf[0]) + from_hex(buf[1]);
    if (calc_cksum != device_cksum) {
        return 0;
    }

    return 1;
}

void parse_coord(const char *str)
{
    cur_lat = 0;
    cur_lat += parsedecn(str, 4);
    str += strlen("llll.");
    cur_lat *= 100000;
    cur_lat += parsedecn(str, 5);
    str += strlen("lllll,");
    if (*str == 'S') {
        cur_lat *= -1;
    }
    str += strlen("a,");

    cur_lon = 0;
    cur_lon += parsedecn(str, 5);
    str += strlen("yyyyy.");
    cur_lon *= 100000;
    cur_lon += parsedecn(str, 5);
    str += strlen("yyyyy.");
    if (*str == 'W') {
        cur_lon *= -1;
    }

    //printf("position %ld %ld\n", cur_lat, cur_lon);
}

void parse_nmea(char *buf)
{
    uint8_t i;
    struct tm tm;

    if (strstart(buf,"$GPRMC")){
        if (!checksum(buf)){
            puts_P(PSTR("nmea err"));
            return;
        }
        for (i = 0; i < 2; i++) {
            while (*buf != ',') buf++; /* go to next comma */
            buf++;
        }
        if (*buf == 'V'){
            /* invalid fix, ignore it */
            return;
        }
        buf += 2; /* this puts us at the start of the lat */
        parse_coord(buf);
    } else if (strstart(buf,"$GPGSV")){
        //$GPGSV,3,1,10,21,46,218,,27,50,041,,26,01,069,,22,31,316,*79
        if (!checksum(buf)){
            puts_P(PSTR("nmea xor"));
            return;
        }
        for(i=0;i<3;i++){
            while(*buf != ',') buf++; /* first comma */
            buf++;
        }
        num_satellites = parsedec2(buf);
        //printf("num sat: %d\n", num_satellites);
    } else if (strstart(buf, "$GPZDA")){
        /*$GPZDA,hhmmss.ss,dd,mm,yyyy,xx,yy*CC */
        if (!checksum(buf)){
            puts_P(PSTR("nmea xor"));
            return;
        }
        buf += strlen("$GPZDA") + 1;
        if (buf[0] == ','){
            //invalid date fix
            return;
        }
        //printf("date fix: '%s'\n", buf);
        tm.tm_hour = parsedec2(buf);
        buf += strlen("hh");
        tm.tm_min = parsedec2(buf);
        buf += strlen("mm");
        tm.tm_sec = parsedec2(buf);
        buf += strlen("ss.ss,"); /* ignore fractional second */
        tm.tm_mday = parsedec2(buf);
        buf += strlen("dd,");
        tm.tm_mon = parsedec2(buf) - 1; /* we want 0-11 instead of 1-12 */
        buf += strlen("mm,");
        tm.tm_year = parsedecn(buf, 4) - 1900;
        //const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        //printf("%s %d %d:%02d:%02d.%d UTC %d\n", months[tm.tm_mon], tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_year);
        cur_time = mktime(&tm);
        //printf("time: %lu\n", cur_time);
    } else {
        return;
    }
}
