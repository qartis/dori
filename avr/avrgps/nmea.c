#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

inline uint8_t strstart(const char *a, const char *b){
    return strncmp(a, b, strlen(b)) == 0;
}

inline uint8_t parsedec2(const char *buf){
    return (buf[0]-'0')*10 + (buf[1]-'0');
}

inline uint16_t parsedec4(const char *buf){
    return (buf[0]-'0')*1000 +
           (buf[1]-'0')*100  +
           (buf[2]-'0')*10   +
           (buf[3]-'0');
}

uint8_t from_hex(char a){
    if (isupper(a)) {
        return a - 'A' + 10;
    } else if (islower(a)) {
        return a - 'a' + 10;
    } else {
        return a - '0';
    }
}

uint16_t checksum(char *buf){
    uint8_t device_cksum;
    uint8_t calc_cksum = 0;

    buf++; //skip the $

    while (*buf != '\0' && *buf != '*') {
        calc_cksum ^= *buf++;
    }
    if (*buf != '*') return 0;

    buf++; //skip the *

    device_cksum = 16 * from_hex(buf[0]) + from_hex(buf[1]);
    if (calc_cksum != device_cksum){
        return 0;
    }

    return 1;
}

void parse_coord(char *str){
    char lat_dir, lon_dir;
    int lat_deg, lat_min;
    int lon_deg, lon_min;
    unsigned int lat_sec, lon_sec;

    printf("original: '%s'\n", str);
    sscanf(str,"%d.%u,%c,%d.%u,%c",&lat_deg,&lat_sec,&lat_dir,&lon_deg,&lon_sec,&lon_dir);
    printf("dirs: %c %c\n", lat_dir, lon_dir);
    lat_min = lat_deg % 100;
    lat_deg /= 100;
    lon_min = lon_deg % 100;
    lon_deg /= 100;
    if (lat_dir == 'S'){
        lat_deg *= -1;
    }
    if (lon_dir == 'W'){
        lon_deg *= -1;
    }
    unsigned lat_frac = lat_min * 1000 / 6;
    unsigned lon_frac = lon_min * 1000 / 6;
    lat_frac += lat_sec/600;
    lon_frac += lon_sec/600;
    uint32_t lat_f = lat_min * 1000000 / 6;
    uint32_t lon_f = lon_min * 1000000 / 6;
    lat_f += (lat_sec * 10) / 6;
    lon_f += (lon_sec * 10) / 6;
    float lat_float = (float)lat_deg + ((float)lat_min + (float)(lat_sec / 100000.0))/60.0;
    float lon_float = (float)lon_deg + ((float)lon_min + (float)(lon_sec / 100000.0))/60.0;
    printf("Original: %d %u.%u, %d %u.%u\n", lat_deg, lat_min, lat_sec, lon_deg, lon_min, lon_sec);
    printf("16 bit fixed point: %d.%04d %d.%04d\n", lat_deg, lat_frac, lon_deg, lon_frac);
    printf("32 bit fixed point: %d.%07lld %d.%07lld\n", lat_deg, lat_f, lon_deg, lon_f);
    printf("Single precision float: %f %f\n", lat_float, lon_float);
}


void parse_nmea(char *buf){
    static uint8_t num_satellites = -1;
    uint8_t i;
    if (strstart(buf,"$GPRMC")){
        if (!checksum(buf)){
            printf("invalid packet: %s\n",buf);
            return;
        }
        for(i=0;i<2;i++){
            while(*buf != ',') buf++; /* go to next comma */
            buf++;
        }
        if (*buf == 'V'){
            //can't remember what this means
            return;
        }
        buf += 2; /* this puts us at the start of the lat */
        parse_coord(buf);
    } else if (strstart(buf,"$GPVTG")){
        //$GPVTG,149.23,T,,M,0.271,N,0.502,K,A*33
        if (!checksum(buf)){
            printf("invalid packet: %s\n",buf);
            return;
        }
        for(i=0;i<7;i++){
            while(*buf != ',') buf++; /* go to next comma */
            buf++;
        }
        if (*buf == ',') return;
        uint8_t kmh = atoi(buf);
        printf("Speed: %d km/h\n",kmh);
    } else if (strstart(buf,"$GPGSV")){
        //$GPGSV,3,1,10,21,46,218,,27,50,041,,26,01,069,,22,31,316,*79
        if (!checksum(buf)){
            printf("invalid packet: %s\n",buf);
            return;
        }
        for(i=0;i<3;i++){
            while(*buf != ',') buf++; /* first comma */
            buf++;
        }
        uint8_t cur_satellites = atoi(buf);
        if (num_satellites != cur_satellites){
            num_satellites = cur_satellites;
            printf("num satellites: %d\n", num_satellites);
        }
    } else if (strstart(buf, "$GPZDA")){
        /*$GPZDA,hhmmss.ss,dd,mm,yyyy,xx,yy*CC */
        if (!checksum(buf)){
            printf("invalid packet: %s\n",buf);
            return;
        }
        buf += strlen("$GPZDA") + 1;
        if (buf[0] == ','){
            //invalid date fix
            return;
        }
        printf("date fix: '%s'\n", buf);
        uint8_t hrs = parsedec2(buf);
        buf += 2;
        uint8_t min = parsedec2(buf);
        buf += 2;
        uint8_t sec = parsedec2(buf);
        buf += 2;
        buf += 1; /* the dot between ss.ss */
        uint8_t sec_frac = parsedec2(buf);
        buf += 2;
        buf += 1; /* the comma after ss.ss */
        uint8_t day = parsedec2(buf);
        buf += 2;
        buf += 1; /* the comma between dd and mm */
        uint8_t mon = parsedec2(buf) - 1; /* we want 0-11 instead of 1-12 */
        buf += 2;
        buf += 1; /* the comma between mm and yyyy */
        uint16_t year = parsedec4(buf);
        const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
        printf("%s %d %d:%02d:%02d.%d UTC %d\n", months[mon], day, hrs, min, sec, sec_frac, year);
    } else {
        return;
        printf("UNKNOWN ");
        puts(buf);
    }
}
