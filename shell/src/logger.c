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

int main()
{
    init_gateway_connection();

    int i;
    int rc;

    uint8_t type, id, len;
    uint16_t sensor;
    uint8_t data[MAX_DATA_LEN];

    while(1) {
        rc = read(gatewayfd, &type, 1);
        if(rc < 1) return -1;

        rc = read(gatewayfd, &id, 1);
        if(rc < 1) return -1;

        rc = safe_read(gatewayfd, &sensor, 2);
        if(rc < 2) return -1;

        rc = read(gatewayfd, &len, 1);
        if(rc < 1) return -1;

        rc = safe_read(gatewayfd, data, len);
        if(rc < len) return -1;

        printf("DORI sent frame: %s [%x] %s [%x] %s [%x] %d [ ",
               type_names[type],
               type,
               id_names[id],
               id,
               sensor_names[sensor],
               sensor,
               len);

        for(i = 0; i < len; i++) {
            printf("%02x ", data[i]);
        }
        printf("]\n");
    }

    return 0;
}

