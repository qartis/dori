#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include "candefs.h"

unsigned msg_type = 1; // TYPE_VALUE_EXPLICIT = 1,

typedef struct {
    unsigned seconds;
    unsigned char id;
} parms;

#define PORT 53
#define SERVER "127.0.0.1"

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

fd_set masterfds;

int sockfd;
int input_buf_rc = 0;
char input_buf[256] = { 0 };

// CAN frame:
// first byte type
// second byte, upper 3 bits + lower 2 bits = ID
// third + fourth byte = exended data bytes
// fifth byte, lower 4 bits = packet length (doesn't include exended data)
// next [packet length] bytes = data bytes

int str_starts_with(char *s1, char *s2) {
    return strncmp(s1, s2, strlen(s2)) == 0;
}

void write_message(unsigned char type, unsigned char id, unsigned char data[10]) {
    unsigned char CAN_frame_bytes[12] = { 0 };
    CAN_frame_bytes[0] = type;
    CAN_frame_bytes[1] = id;
    memcpy(&CAN_frame_bytes[2], data, 10);
    int i = 0;
    printf("wrote %d id message\n", id);
    for(; i < 12; i++) {
        printf("%d\n", CAN_frame_bytes[i]);
    }
    write(sockfd, CAN_frame_bytes, 10);
}

void process_message(char *msg) {
    if(str_starts_with(msg, "drive")) {
        long drive_left, drive_right;
        sscanf(msg, "drive %ld %ld", &drive_left, &drive_right);
        long drive_max = (MAX(drive_left, drive_right));
        printf("driving for %ld ms\n", drive_max);
        drive_max *= 1000; // convert to microseconds
        usleep(drive_max);
        printf("finished driving\n");

        char output[128];

        sprintf(output, "drive complete");

        unsigned char data[] = { 1, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0 };
        write_message(msg_type, ID_DRIVE, data);
        write(sockfd, output, strlen(output));
    }
}

void *send_message(void* data) {
    parms *p = (parms*)data;
    while(1) {
        struct timeval timeout;

        timeout.tv_sec = p->seconds;
        fd_set readfds = masterfds;
        int rc = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if(rc > 0) {
            printf("rc > 0\n");
            FD_CLR(sockfd, &readfds);
            rc = read(sockfd, &input_buf[input_buf_rc], 256);
            if(input_buf[input_buf_rc + rc - 1] == '\n') {
                input_buf[input_buf_rc + rc] = '\0';
                process_message(input_buf);
            }
            else {
                input_buf_rc += rc;
            }
        }

        switch(p->id) {
        case ID_LASER:
            {
                // laser data: 2 byte angle (sensor plate pos), 2 byte distance (in mm)
                unsigned int angle = rand() % 360;
                unsigned int dist = rand() % 7000;

                // LSB to MSB
                unsigned char data[] = { (angle & 0x00FF), ((angle & 0xFF00) >> 8),
                                         (dist  & 0x00FF), ((dist  & 0xFF00) >> 8),
                                         0, 0, 0, 0, 0, 0
                                       };
                write_message(msg_type, p->id, data);
            }
            break;
        case ID_GPS:
            break;
        case ID_TEMP:
            {
                // laser data: 2 byte angle (sensor arm pos), 2 byte distance (in mm)
                int deg = rand() % 300;

                // LSB to MSB
                unsigned char data[] = { (deg & 0x00FF),
                                         ((deg & 0xFF00) >> 8),
                                         0, 0, 0, 0, 0, 0, 0, 0,
                };
                write_message(msg_type, p->id, data);
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

    FD_ZERO(&masterfds);
    FD_SET(sockfd, &masterfds);

    srand(time(NULL));

    int i;
    for(i = 0; i <= MAX_MESSAGE_ID; i++) {
        p[i].seconds = 1;//i + 1;
        p[i].id = i;
        pthread_create(&threads[i], NULL, send_message, (void*)&p[i]);
    }

    for(i = 0; i <= MAX_MESSAGE_ID; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
