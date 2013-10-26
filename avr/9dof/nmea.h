extern uint8_t num_satellites;
extern uint32_t cur_time;

int checksum(char *buf);
void parse_coord(char *str,
                 volatile int32_t *cur_lat,
                 volatile int32_t *cur_lon);

uint8_t parse_nmea(char *buf,
                   volatile int32_t *cur_lat,
                   volatile int32_t *cur_lon);
