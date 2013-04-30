#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include "../candefs.h"

unsigned msg_type = 1; // TYPE_VALUE_EXPLICIT = 1,

typedef struct {
    unsigned seconds;
    unsigned char id;
} parms;

#define PORT 53
#define SERVER "127.0.0.1"

int sockfd;

// CAN frame:
// first byte type
// second byte, upper 3 bits + lower 2 bits = ID
// third + fourth byte = exended data bytes
// fifth byte, lower 4 bits = packet length (doesn't include exended data)
// next [packet length] bytes = data bytes


int write_CAN_frame(unsigned char type, unsigned char id, unsigned char ex_data_bytes[2], unsigned char data_len, unsigned char *reg_data_bytes) {
    unsigned char CAN_frame_bytes[13];
    CAN_frame_bytes[0] = type;
    CAN_frame_bytes[1] = ((id & 0b00011100) << 3) | (id & 0b00000011);
    CAN_frame_bytes[2] = ex_data_bytes[0];
    CAN_frame_bytes[3] = ex_data_bytes[1];
    CAN_frame_bytes[4] = data_len & 0x0f;

    printf("sending data\n");
    printf("type: %u\n", CAN_frame_bytes[0]);
    printf("id: %u\n", id);
    printf("ex_data_byte[0]: %u\n", CAN_frame_bytes[2]);
    printf("ex_data_byte[1]: %u\n", CAN_frame_bytes[3]);
    printf("data_len: %u\n", CAN_frame_bytes[4]);

    int i;
    for(i = 0; i < data_len; i++) {
        CAN_frame_bytes[5 + i] = reg_data_bytes[i];
        printf("reg_data_byte[%d]: %u\n", i, reg_data_bytes[i]);
    }
    printf("\n\n\n");

    unsigned num_bytes = (5 * sizeof(unsigned char)) + CAN_frame_bytes[4];

    write(sockfd, CAN_frame_bytes, num_bytes);
}

void *send_message(void* data) {
    parms *p = (parms*)data;
    while(1) {
        sleep(p->seconds);
        switch(p->id) {
        case ID_LASER:
            {
                // laser data: 2 byte angle (sensor plate pos), 2 byte distance (in mm)
                unsigned char ex_data_bytes[] = { 0, 0 };

                unsigned int angle = rand() % 360;
                unsigned int dist = rand() % 7000;
                unsigned char reg_data_bytes[] = { ((angle & 0xFF00) >> 8), ((angle & 0x00FF)),
                                                    ((dist & 0xFF00) >> 8), ((dist & 0x00FF))
                                                 };
                unsigned char data_len = sizeof(reg_data_bytes);
                write_CAN_frame(msg_type, p->id, ex_data_bytes, data_len, reg_data_bytes);
            }
            break;
        case ID_GPS:
            break;
        case ID_TEMP:
            {
                // laser data: 2 byte angle (sensor arm pos), 2 byte distance (in mm)
                unsigned char ex_data_bytes[] = { 0, 0 };

                int deg = rand() % 300;
                unsigned char reg_data_bytes[] = { ((deg & 0xFF00) >> 8), ((deg & 0x00FF)) };
                unsigned char data_len = sizeof(reg_data_bytes);
                write_CAN_frame(msg_type, p->id, ex_data_bytes, data_len, reg_data_bytes);
            }
            break;
        case ID_TIME:
            break;
        case ID_LOGGER:
            break;
        }
    }
}

int main() {
    pthread_t threads[MAX_MESSAGE_ID];
    parms p[MAX_MESSAGE_ID];

    struct sockaddr_in serv_addr;
    struct hostent *server = NULL;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        printf("error opening socket\n");
        exit(0);
    }

    server = gethostbyname(SERVER);
    if(server == NULL) {
        printf("error, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(PORT);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("error connecting to Gateway\n");
        exit(0);
    }

    srand(time(NULL));

    int i;
    for(i = 0; i <= MAX_MESSAGE_ID; i++) {
        p[i].seconds = 1;
        p[i].id = i; ((i & 0b00011100) << 3) | (0b00000011);
        pthread_create(&threads[i], NULL, send_message, (void*)&p[i]);
    }

    for(i = 0; i < MAX_MESSAGE_ID; i++) {
        pthread_join(threads[i], NULL);
    }
}
