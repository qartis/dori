/* 80 lines of text, plus possible \r\n, plus NULL */
#define BUFLEN 80+2+1

enum nmea_value {
    NMEA_UNKNOWN = 0,
    NMEA_TIMESTAMP,
    NMEA_COORDS,
    NMEA_NUM_SATS
};

struct nmea_data_t {
    enum nmea_value tag;

    union {
        uint8_t num_sats;

        struct {
            int32_t cur_lat;
            int32_t cur_lon;
        };

        uint32_t timestamp;
    };
};

uint16_t checksum(const char *buf);
void parse_coord(const char *str, struct nmea_data_t *data);

struct nmea_data_t parse_nmea(const char *buf);
