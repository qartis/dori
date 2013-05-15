#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <termio.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 256
#define AT_NO_ECHO 0
#define AT_SYNC_BAUD 0

int modemFD;

volatile int flag_ok;
volatile int flag_error;
volatile int flag_nocarrier;
volatile int flag_connect;
volatile int flag_http;
volatile int received;
volatile int ignore;

inline int strcasestart(const char *buf1, const char *buf2)
{
    return strncasecmp(buf1, buf2, strlen(buf2)) == 0;
}

int streq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

void* interrupt(void *arg)
{
    (void)arg;
    int rc;
    char c;
    char buf[128];
    int count = 0;
    static int http_len;

    for (;;) {
        rc = read(modemFD, &c, 1);
        if (rc < 0) {
            perror("read");
            exit(1);
        }

        //printf("%x %c\n", c, isprint(c) ? c : '?');

        received = 1;

        if (c == '\r') {
            if (!ignore) {
                if (streq(buf, "OK")) {
                    flag_ok = 1;
                    //printf("FLAG: ok\n");
                } else if (streq(buf, "ERROR")) {
                    flag_error = 1;
                    //printf("FLAG: error\n");
                } else if (streq(buf, "NO CARRIER")) {
                    flag_nocarrier = 1;
                    //printf("FLAG: nocarrier\n");
                } else if (streq(buf, "CONNECT")) {
                    flag_connect = 1;
                    //printf("FLAG: connect\n");
                } else if (strncmp(buf, "HTTP", 4) == 0) {
                    char *status = buf + strlen("HTTP/1.0 ");
                    flag_http = atoi(status);
                    //printf("http status: %d\n", flag_http);
                } else if (strcasestart(buf, "Content-length")) {
                    char *length = buf + strlen("Content-length: ");
                    if (flag_http) {
                        http_len = atoi(length);
                    }
                } else if (flag_http > 0 && http_len > 0 && count == 0) {
                    /* get rid of last \n from http header */
                    read(modemFD, &c, 1);

             //       printf("HTTP PAYLOAD:\n");
                    int i;
                    for (i = 0; i < http_len; i++) {
                        read(modemFD, &c, 1);
                        /*
                        if (isprint(c)) {
                            putchar(c);
                        } else {
                            printf("[%x]", c);
                        }
                        */
                    }

                    //printf("\nDONE HTTP PAYLOAD\n");
                    flag_http = 0;
                } else if (count > 0) {
            //        printf("unrecognized response: '%s'\n", buf);
                }
            }
            count = 0;
            buf[0] = '\0';
        } else if (c != '\n' && c != '\0') {
            buf[count] = c;
            buf[count + 1] = '\0';
            count++;
        }
    }
}

size_t slow_write(int fd, char *buf, size_t count)
{
    unsigned i = 0;
    size_t rc;

    flag_ok = 0;
    flag_error = 0;
    flag_nocarrier = 0;
    flag_connect = 0;
    flag_http = 0;

    while (i < count) {
        rc = write(fd, (uint8_t *)buf + i, 1);
        if (rc < 1) {
            perror("write");
            exit(1);
        }
        usleep(500); // was set to 5000 for file transfer test

        if (buf[i] == '\r') {
            usleep(500);
        }

        i++;
    }

    return i;
}

void upload(const char *filename)
{
    struct stat st;
    int rc;
    size_t sent;
    unsigned progress;
    char buf[128];
    int fd;
    struct timeval start, end;

    rc = stat(filename, &st);
    if (rc < 0) {
        perror("stat");
        exit(1);
    }

    sprintf(buf,
        "POST http://h.qartis.com/dori/hey HTTP/1.0\r\n"
        "Host: h.qartis.com\r\n"
        "Expect: 100-continue\r\n"
        "Content-length: %lu\r\n\r\n", st.st_size);
    slow_write(modemFD, buf, strlen(buf));

    fd = open(filename, O_RDONLY);
    sent = 0;
    progress = 0;
    flag_http = 0;
    received = 0;

    gettimeofday(&start, NULL);

    while (received == 0) {
        rc = read(fd, buf, 1);
        if (rc < 0) {
            perror("read file");
            exit(1);
        } else if (rc == 0) {
            break;
        }

        slow_write(modemFD, buf, rc);
        sent += rc;
        if ((sent * 10 / st.st_size) > progress) {
            progress++;
            fprintf(stderr, "%d ", progress * 10);
        }
    }

    if (received != 0) {
        printf("upload terminated\n");
    }

    gettimeofday(&end, NULL);

    int elapsed = end.tv_sec - start.tv_sec;
    if (elapsed == 0)
        elapsed = 1;

    printf("\n");
    printf("uploaded %lu bytes in %d seconds, %ld bytes/sec\n", st.st_size, elapsed, st.st_size / elapsed);
}

void parse_command(int modemFD, char *command)
{
    char cmd[BUFFER_SIZE];
    char arg1[BUFFER_SIZE];
    char arg2[BUFFER_SIZE];
    char buf[128];
    char *ptr;
    int num_args;

    int length = strlen(command);

    strcpy(cmd, command);

    // break up the commands and the arguments
    ptr = strtok(cmd, " ");
    if (ptr != NULL) {
        strcpy(cmd, ptr);
    }

    num_args = 0;
    ptr = strtok(NULL, " ");
    if (ptr != NULL) {
        strcpy(arg1, ptr);
        num_args++;

        ptr = strtok(NULL, " ");
        if (ptr != NULL) {
            strcpy(arg2, ptr);
            num_args++;
        }
    }

    ignore = 1;
    slow_write(modemFD, "AT\r", 3);
    usleep(50000);
    ignore = 0;

    if (streq(cmd, "+++")) {
        sleep(1);
        slow_write(modemFD, "+++", 3);
    } else if (strcasecmp(cmd, "put") == 0) {
        if (num_args != 1) {
            printf("usage: put [filename]\n");
            return;
        }

        sprintf(buf, "AT+CGDCONT=1,\"IP\",\"goam.com\",,\r");
        slow_write(modemFD, buf, strlen(buf));
        while (!flag_ok && !flag_error) {}

        sprintf(buf, "AT%%CGPCO=1,\"PAP,wapuser1,wap\",2\r");
        slow_write(modemFD, buf, strlen(buf));
        while (!flag_ok) {}

        sprintf(buf, "AT$DESTINFO=\"10.128.1.69\",1,80,0\r");
        slow_write(modemFD, buf, strlen(buf));
        while (!flag_ok) {}

        sprintf(buf, "ATD*97#\r");
        slow_write(modemFD, buf, strlen(buf));

        while (!flag_ok) {
            usleep(100000);
        }
        usleep(500000);
        
        if (!flag_ok || flag_error || flag_nocarrier || flag_connect) {
            printf("error connecting\n");
            return;
        }
        
        printf("connected!\n");

        upload(arg1);

        uint16_t retry = 255;
        while (flag_http == 0 && --retry) {
            usleep(100000);
            //printf("retry %d\n", retry);
        }

        if (retry == 0) {
            printf("no response!\n");
        } else if (flag_http == 200) {
            printf("upload success\n");
        } else {
            printf("http error: %d\n", flag_http);
        }
    } else if (strcasestart(command, "AT")) {
        command[length] = '\r';
        command[length + 1] = '\0';
        slow_write(modemFD, command, strlen(command));
        usleep(100000);
        if (flag_error) {
            printf("error\n");
        } else if (flag_ok) {
            printf("ok\n");
        } else {
            printf("wtf!\n");
            exit(1);
        }
        //printf("flags: %s %s %s\n", flag_ok ? "ok" : "", flag_error ? "error" : "", flag_nocarrier ? "nocarrier" : "", flag_connect ? "connect" : "");
    } else {
        printf("error: unrecognized command: %s\n", cmd);
    }
}

int modem_init()
{
    struct termios tp;

    modemFD = open("/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller-if00-port0", O_RDWR | O_NOCTTY);
    if (modemFD < 0) {
        perror("open modemFD");
        exit(1);
    }

    tcflush(modemFD, TCIOFLUSH);
    ioctl(modemFD, TCGETS, &tp);
    cfmakeraw(&tp);
    cfsetspeed(&tp, 9600);
    ioctl(modemFD, TCSETS, &tp);

    return modemFD;
}

int main()
{
    char buf[256];
    int rc;
    
    modem_init();

    pthread_t thread;

    pthread_create(&thread, NULL, interrupt, NULL);

    ignore = 1;
    slow_write(modemFD, "ATE0\r", strlen("ATE0\r"));
    usleep(100000);
    ignore = 0;

    for (;;) {
        fprintf(stderr, "> ");
        rc = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (rc < 1) {
            perror("read stdin");
            exit(1);
        }
        buf[rc - 1] = '\0';
        parse_command(modemFD, buf);
    }
    return 0;
}
