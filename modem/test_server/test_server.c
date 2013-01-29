#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <openssl/md5.h>

#define NUM_BUFS 6

char *commands[] = {
    "cmd ping 60\n",
    "cmd ping 30\n",
    "cmd ping 120\n",
    "cmd ping 60\n",
    "cmd ping 30\n",
    "cmd ping 40\n",
};

int ping_counter;
int total_out_of_sync_pings;

int main() {

    setbuf(stdout, NULL);

    int tcpfd, newfd, maxfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    socklen_t len;
    fd_set master;
    fd_set readfds;
    fd_set writefds;
    int optval, rc;

    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpfd == -1) {
        perror("tcp socket");
        return -1;
    }

    optval = 1;
    setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    memset(&servaddr, '\0', sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(30000);

    rc = bind(tcpfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (rc == -1) {
        perror("bind: tcp");
        return -1;
    }

    rc = listen(tcpfd, 1);
    if (rc == -1) {
        perror("listen: tcp");
        return -1;
    }

    time_t cur_time;
    char * time_str;

    char buf[256];
    int buf_index = 0;
    //char command_buffers[NUM_BUFS][256];

    FD_ZERO(&master);
    FD_ZERO(&writefds);
    FD_SET(tcpfd, &master);
    FD_SET(0, &master);
    maxfd = tcpfd;

    for (;;) {
        readfds = master;

        rc = select(maxfd + 1, &readfds, &writefds, NULL, NULL);
        if (rc == -1) {
            perror("select");
            return -1;
        }
        /*
        if(FD_ISSET(0, &readfds)) { // STDIN
            int rcv = read(0, command_buffers[buf_index], sizeof(command_buffers[buf_index]));
            command_buffers[buf_index][rcv] = '\0';
            printf("'%s' entered in buffer %d\n", command_buffers[buf_index], buf_index);
            buf_index = (buf_index + 1) % 4;
        }
        else */if(FD_ISSET(tcpfd, &readfds)) {
            len = sizeof(clientaddr);
            newfd = accept(tcpfd, (struct sockaddr *)&clientaddr, &len);
            setbuf(fdopen(newfd, "r"), NULL);
            if (newfd == -1) {
                perror("accept: tcp");
                return -1;
            }
            else {
                printf("connection established\n");
            }

            if(newfd > maxfd) {
                maxfd = newfd;
            }

            /*
            if(buf_index > 0) {
                FD_SET(newfd, &writefds);
            }
            */

            FD_SET(newfd, &master);
        }
        else if(FD_ISSET(newfd, &readfds)) {
            rc = read(newfd, buf, sizeof(buf));
            if(rc <= 0) {
                printf("client disconnected\n");
                FD_CLR(newfd, &master);
                newfd = -1;
            }
            else {
                cur_time = time(NULL);
                time_str = ctime(&cur_time);
                time_str[strlen(time_str) - 1] = '\0'; // get rid of the newline
                buf[rc] = '\0';

                char *ptr = strstr(buf, "ping ");
                if(ptr) {
                    printf("%s [%s]\n", buf, time_str);
                    int ping = atoi(ptr + strlen("ping "));
                    if(ping != ping_counter) {
                        total_out_of_sync_pings++;

                        printf("ping index not in sync: modem: %d, server: %d, total times out of sync: %d\n", ping, ping_counter, total_out_of_sync_pings);
                        ping_counter = ping;
                    }
                    if(ping_counter > 0 && (ping_counter % 5) == 0) {
                        FD_SET(newfd, &writefds);
                    }
                    ping_counter++;
                }
            }
        }
        else if(FD_ISSET(newfd, &writefds)) {
            printf("sending command: '%s'", commands[buf_index]);
            write(newfd, commands[buf_index], strlen(commands[buf_index]));
            buf_index++;
            buf_index %= 16;
            FD_CLR(newfd, &writefds);
        }
    }

    return 0;
}
