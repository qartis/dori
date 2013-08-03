extern uint8_t num_satellites;
extern uint32_t cur_time;
extern int32_t cur_lat;
extern int32_t cur_lon;

int checksum(char *buf);
void parse_coord(char *str);
uint8_t parse_nmea(char *buf);
