#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

inline uint8_t strstart(const char *buf1, const char *buf2){
    return strncmp(buf1,buf2,strlen(buf2)) == 0;
}

inline uint8_t parsedec(const char *buf){
    return (buf[0]-'0');
}

inline uint8_t parsedec2(const char *buf){
    return (buf[0]-'0')*10 + (buf[1]-'0');
}

inline uint16_t parsedec3(const char *buf){
    return (buf[0]-'0')*100 +
           (buf[1]-'0')*10  +
           (buf[2]-'0')*1;
}

inline uint16_t parsedec4(const char *buf){
    return (buf[0]-'0')*1000 +
           (buf[1]-'0')*100  +
           (buf[2]-'0')*10   +
           (buf[3]-'0');
}

uint8_t from_hex(char a){
    if (a >= 'A' && a <= 'F'){
        return a - 'A' + 10;
    } else if (a >= 'a' && a <= 'f'){
        return a - 'a' + 10;
    } else {
        return a - '0';
    }
}

uint8_t checksum(const char *buf){
    buf++; //skip the $
    uint8_t calculated_checksum = 0;
    while(*buf != '\0' && *buf != '*'){
        calculated_checksum ^= *buf++;
    }
    if (*buf != '*') return 0;
    buf++; //skip the *
    uint8_t device_checksum = (from_hex(buf[0]) << 4) + from_hex(buf[1]);
    if (calculated_checksum != device_checksum){
        return 0;
    }
    return 1;
}

struct coord {
    int8_t deg;
    uint32_t frac;
};

/* this updates *nmea to point at the start of the next sentence */
struct coord parse_coord(const char **nmea){
    const char *buf = *nmea;
    struct coord coord;

    if (buf[4] == '.'){
        coord.deg = parsedec2(buf);
        buf+=2;
    } else {
        coord.deg = parsedec3(buf);
        buf+=3;
    }

    coord.frac = 0;
    while(*buf != ','){
        if (*buf != '.'){
            coord.frac *= 10;
            coord.frac += parsedec(buf);
        }
        buf++;
    }
    buf++;
    coord.frac *= 700; /* 700/42 equals 1000/60 */
    coord.frac /= 42;

    if (*buf == 'S' || *buf == 'W'){
        coord.deg *= -1;
    }
    buf += 2;
    *nmea = buf;
    return coord;
}

void parse_nmea(const char *buf){
    uint8_t i;
    if (strstart(buf,"$GPRMC")){
        if (!checksum(buf)){
            //printf("invalid: %s\n",buf);
            return;
        }
        for(i=0;i<2;i++){
            while(*buf != ',') buf++;
            buf++;
        }
        if (*buf != 'A'){ /* 'A' means A Valid Fix */
            return;
        }
        //buf += 2; /* this puts us at the start of the lat */
        /*
        struct coord c;
        c = parse_coord(&buf);
        printf("%d.%8lu,", c.deg, c.frac);
        c = parse_coord(&buf);
        printf("%d.%8lu\n", c.deg, c.frac);
        */
    }
}
