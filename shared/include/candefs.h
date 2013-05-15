#define MAX_MESSAGE_ID 7

typedef enum {
    ID_ANY   = 0,
    ID_PING  = 1,
    ID_PONG  = 2,
    ID_LASER = 3,
    ID_GPS   = 4,
    ID_TEMP  = 5,
    ID_TIME  = 6,
    ID_LOGGER = 7,
    ID_DRIVE = 8,
    ID_INVALID = 0xff
} id;


