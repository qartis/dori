#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <time.h>
#include <signal.h>
#include "can.h"
#include "can_names.h"
#include "shell.h"

int main(int argc, char *argv[])
{
    if(argc < 3 || argc > 11)
    {
        printf("Usage: send [D]? [TYPE] [ID.SENSOR] [DATA]\n");
        exit(0);
    }

    uint8_t type, id, len;
    uint8_t data[MAX_DATA_LEN];
    uint16_t sensor;

    // If the first arg is a single D, then we connect to gateway on port 53,
    // in order to impersonate DORI
    if(strlen(argv[1]) == 1 && argv[1][0] == 'D') {
        gateway_port = 53;
        argv = &argv[1];
        argc -= 1;
    }

    init_gateway_connection();

    // skip over the executable filename
    argv = &argv[1];
    argc -= 1;

    parse_can_packet_from_stdin(argc, argv, &type, &id, &sensor, &len, data);

    transmit_can_packet(type, id, sensor, len, data);

    // switch based on the type
    switch(type) {
    case TYPE_dcim_read:
        process_dcim_read();
        break;
    case TYPE_file_read:
        process_file_read();
        break;
    case TYPE_file_tree:
        process_file_tree();
        break;
    }
    return 0;
}

