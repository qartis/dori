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
#include <errno.h>

#define BUFFER_SIZE 256
#define AT_NO_ECHO 0
#define AT_SYNC_BAUD 0

int modemFD;

volatile int delay;
volatile int ping_time = 10;
volatile int ping_counter;

volatile int uploading;
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
        usleep(900); // was set to 5000 for file transfer test

        if (buf[i] == '\r') {
            usleep(900);
        }

        if(uploading) {
            printf("sleeping..\n");
            sleep(delay);
            printf("woke up..\n");
        }

        i++;
    }

    return i;
}


void disconnect()
{
    uploading = 0;
    printf("disconnecting..\n");
    sleep(4);
    slow_write(modemFD, "+++", 3);
    sleep(2);
    slow_write(modemFD, "+++", 3);
    sleep(1);

    uint16_t retry = 100;
    while (!flag_nocarrier && --retry) {
        usleep(100000);
        //printf("retry %d\n", retry);
    }

    if (retry == 0) {
        printf("failed to disconnect\n");
    } else {
        printf("successfully disconnected\n");
    }
}

void connect()
{
    char buf[128];

    for(;;) {
        printf("preparing to connect\n");

        //autobaud
        slow_write(modemFD, "AT\r", 3);
        usleep(50000);

        sprintf(buf, "AT+CGDCONT=1,\"IP\",\"internet.fido.ca\",,\r");
        slow_write(modemFD, buf, strlen(buf));
        uint8_t retry = 255;
        while ((!flag_ok && !flag_error) && --retry) {
            usleep(50000);
        }

        if(retry == 0) {
            printf("didn't get an OK from the modem when setting AT+CGDCONT\n");
            printf("trying to disconnect in case we're in a weird state\n");
            disconnect();
            usleep(50000);
            continue;
        }

        sprintf(buf, "AT%%CGPCO=1,\"PAP,fido,fido\",2\r");
        slow_write(modemFD, buf, strlen(buf));

        retry = 255;
        while ((!flag_ok) && --retry) {
            usleep(50000);
        }

        if(retry == 0) {
            printf("didn't get an OK from the modem when setting AT%%CGPCO\n");
            printf("trying to disconnect in case we're in a weird state\n");
            disconnect();
            usleep(50000);
            continue;
        }

        sprintf(buf, "AT$DESTINFO=\"h.qartis.com\",1,30000,1\r");
        retry = 255;
        slow_write(modemFD, buf, strlen(buf));
        while ((!flag_ok) && --retry) {
            usleep(50000);
        }

        if(retry == 0) {
            printf("didn't get an OK from the modem when setting AT$DESTINFO\n");
            printf("trying to disconnect in case we're in a weird state\n");
            disconnect();
            usleep(50000);
            continue;
        }

        sprintf(buf, "ATD*97#\r");
        slow_write(modemFD, buf, strlen(buf));

        printf("connecting...\n");

        while (!flag_ok) {
            usleep(100000);
        }
        usleep(500000);

        if (!flag_ok || flag_error || flag_nocarrier || flag_connect) {
            printf("error while dialing\n");
            disconnect();
            usleep(50000);
            continue;
        }

        printf("connected!\n");
        break;
    }
}

void ping()
{

    connect();
    char pingbuf[128];

    sprintf(pingbuf, "ping %d", ping_counter++);

    printf("sending %s\n", pingbuf);

    uploading = 1;
    slow_write(modemFD, pingbuf, strlen(pingbuf));
    uploading = 0;

    // keep the connection open for a few seconds
    // to give the server a chance to respond
    sleep(5);

    disconnect();
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
        sleep(1);
        slow_write(modemFD, "+++", 3);

        perror("stat");
        exit(1);
    }

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
            printf("%d ", progress * 10);
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

void parse_command(char *command)
{
    char cmd[BUFFER_SIZE];
    char arg1[BUFFER_SIZE];
    char arg2[BUFFER_SIZE];
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

    if (streq(cmd, "+++")) {
        sleep(1);
        slow_write(modemFD, "+++", 3);
    } else if (strcasecmp(cmd, "put") == 0) {
        if (num_args != 1) {
            printf("usage: put [filename]\n");
            return;
        }

        connect();

        printf("connected! preparing to upload file: %s\n", arg1);

        uploading = 1;
        upload(arg1);
        uploading = 0;

        uint16_t retry = 20;
        while (--retry) {
            usleep(100000);
        }

        if (retry == 0) {
            printf("file upload complete!\n");
        } else {
            printf("file upload error\n");
        }

        disconnect();

    } else if(strcasecmp(command, "connect") == 0) {
        connect();
    } else if(strcasestart(command, "delay")) {
        if (num_args == 0) {
            printf("delay: %d\n", delay);
        } else if(num_args == 1) {
            delay = atoi(arg1);
            printf("delay set to %d\n", delay);
        }
        else {
            printf("usage: delay (int seconds)\n");
        }
    } else if(strcasestart(command, "ping")) {
        if (num_args == 0) {
            printf("ping: %d\n", ping_time);
        } else if(num_args == 1) {
            ping_time = atoi(arg1);
            printf("ping set to %d\n", ping_time);
        }
        else {
            printf("usage: ping (int seconds)\n");
        }

    } else if (strcasestart(command, "AT")) {
        slow_write(modemFD, "AT\r", 3);
        usleep(50000);
        command[length] = '\r';
        command[length + 1] = '\0';
        slow_write(modemFD, command, strlen(command));
        usleep(100000);
        if (flag_error) {
            printf("error\n");
        } else if (flag_ok) {
            printf("ok\n");
        } else {
            printf("no response, are you in a data call?\n");
        }
        //printf("flags: %s %s %s\n", flag_ok ? "ok" : "", flag_error ? "error" : "", flag_nocarrier ? "nocarrier" : "", flag_connect ? "connect" : "");
    } else {
        printf("error: unrecognized command: %s\n", cmd);
    }
}


void* interrupt(void *arg)
{
    (void)arg;
    int rc;
    unsigned char c;
    char buf[128];
    int count = 0;
    //static int http_len;

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
                } else if (count > 0) {
                    //printf("unrecognized response: '%s'\n", buf);
                }
            }
            count = 0;
            buf[0] = '\0';
        } else if (c != '\n' && c != '\0') {
            buf[count] = c;
            buf[count + 1] = '\0';
            count++;
        } else if (c == '\n') {
            buf[count] = '\0';
            count = 0;
            if(strcasestart(buf, "cmd ")) {
                char *cmd = &buf[strlen("cmd ")];
                parse_command(cmd);
            }
        }
    }
}

int modem_init()
{
    struct termios tp;

    printf("initializing modem\n");
    modemFD = open("/dev/serial/by-id//usb-067b_0609-if00-port0", O_RDWR | O_NOCTTY);
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

    slow_write(modemFD, "AT\rAT\rAT\r", 9);
    usleep(100000);
    slow_write(modemFD, "ATE0\r", strlen("ATE0\r"));
    usleep(100000);

    int flags = fcntl(0, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(0, F_SETFL, flags);

    struct timeval start, cur;
    int idle_seconds;
    int prev_idle_seconds;

    gettimeofday(&start, NULL);

    setbuf(stdout, NULL);

    fprintf(stderr, "> ");
    for (;;) {
        rc = read(STDIN_FILENO, buf, sizeof(buf));
        if (rc < 1) {
            switch(errno) {
#if EAGAIN != EWOULDBLOCK
            case EAGAIN:
#endif
            case EWOULDBLOCK:
                gettimeofday(&cur, NULL);
                prev_idle_seconds = idle_seconds;
                idle_seconds = cur.tv_sec - start.tv_sec;

                if(!uploading) {
                    if(idle_seconds != prev_idle_seconds) {
                        printf(".");
                    }
                }
                if(idle_seconds >= ping_time) {
                    // don't send a ping if we're uploading a file
                    if(!uploading) {
                        printf("preparing to send ping...\n");
                        system("date");
                        ping();
                    }
                    gettimeofday(&start, NULL);
                }
                break;
            default:
                perror("read stdin");
                exit(1);
                break;
            }
        }
        else {
            //printf("resetting start time\n");
            if(buf[rc - 1] == '\n') {
                buf[rc - 1] = '\0';
                parse_command(buf);
                fprintf(stderr, "> ");
            }
            gettimeofday(&start, NULL);
        }
    }
    return 0;
}
