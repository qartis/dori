#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <termio.h>
#include <string.h>
#include <unistd.h>

#define AT_ECHO 0
#define AT_SYNC_BAUD 0

static char *baudsync = "at\rat\rat\r";
static char *noecho = "ate0\r";
static char *disconnect_AT = "+++";
static char *connect = "connect";
static char *disconnect = "disconnect";
static char *destinfo = "DESTINFO";
static char *default_server = "qartis.com";
static char *default_port = "30000";
static char *error = "ERROR";
static char *no_carrier = "NO CARRIER";
static char *connected_response = "PONG";

typedef enum {
    WAITING = 0,
    DATA_MODE_CONNECTING,
    DATA_MODE_CONNECTED,
} STATE;

static STATE cur_state;

inline int strcasestart(const char *buf1, const char *buf2)
{
    return strncasecmp(buf1, buf2, strlen(buf2)) == 0;
}

size_t slow_write(int fd, uint8_t * buf, size_t count)
{
    int i = 0;
    size_t rc;

    while (i < count) {
        rc = write(fd, buf + i, 1);
        if (rc < 0) {
            return -1;
        }
        usleep(50);

        if (buf[i] == '\r') {
            usleep(50000);
        }
        i++;
    }

    return count;
}

void eat_leading_newlines(char **str)
{
    while (**str == 0xa || **str == 0xd) {
        (*str)++;
    }
}

void eat_trailing_newlines(char **str)
{
    int i;

    i = strlen(*str) - 1;

    while (i >= 0 && ((*str)[i] == 0xa || (*str)[i] == 0xd)) {
        (*str)[i] = '\0';
        i--;
    }
}

void cleanup_string(char **str)
{
    eat_leading_newlines(str);
    eat_trailing_newlines(str);
}

void parse_response(char *response)
{
    int i;
    int temp;

    cleanup_string(&response);

    printf("< %s\n", response);

    // check for server reply
    if (cur_state == DATA_MODE_CONNECTING) {
        if (strstr(response, connected_response)) {
            printf("connection established\n");
            cur_state = DATA_MODE_CONNECTED;
        }
    }
    if (strstr(response, error)) {
        //printf("error detected\n");
    }
    if (strstr(response, no_carrier)) {
        printf("disconnected\n");
        cur_state = WAITING;
        return;
    }
}

void parse_command(int modemFD, char *command, char *at_commands)
{
    char cmd[256];
    char arg1[256];
    char arg2[256];
    char *ptr;
    int num_args;
    int i;
    int total_output_commands;

    if (command == NULL || at_commands == NULL) {
        printf("error: command or at_commands NULL\n");
        return;
    }

    int length = strlen(command);

    switch (cur_state) {
    case DATA_MODE_CONNECTING:
        return;                 // don't parse commands while we're trying to connect
        break;
    case DATA_MODE_CONNECTED:
        // if we're connected and we entered the disconnect command
        // then we'll handle writing the d/c message
        if (strcasecmp(command, disconnect) == 0) {
            slow_write(modemFD, disconnect_AT, strlen(disconnect_AT));
            fflush(fdopen(modemFD, "r+"));

            sleep(1);           // sleep for a second

            fflush(fdopen(modemFD, "r+"));
            slow_write(modemFD, disconnect_AT, strlen(disconnect_AT));

            at_commands[0] = '\0';
            printf("sent d/c signal\n");
        }
        // otherwise if we just entered data to be sent
        // then just set the output to be the input
        at_commands = command;
        return;
        break;
    default:
        if (strcasestart(command, "AT")) {
            at_commands[length] = '\r';
            at_commands[length + 1] = '\0';
            // we don't need to parse AT commands, return
            return;
        }
        break;
    }

    if (cmd != command) {
        strcpy(cmd, command);
    }
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

    if (strcasecmp(cmd, connect) == 0) {
        if (num_args == 0) {
            strcpy(arg1, default_server);
            strcpy(arg2, default_port);
            num_args = 2;
        } else if (num_args == 1) {
            printf("error: incorrect number of arguments for command: %s\n",
                   cmd);
            return;
        }

        sprintf(at_commands,
                "AT+CGDCONT=1,\"IP\",\"internet.fido.ca\",,\rAT%%CGPCO=1,\"PAP,fido,fido\",2\rAT$DESTINFO=\"%s\",1,%s,1\rATD*97#\r",
                arg1, arg2);

        cur_state = DATA_MODE_CONNECTING;
        printf("attempting to connect to %s:%s\n", arg1, arg2);
    } else {
        printf("error: unrecognized command: %s\n", cmd);
        at_commands[0] = '\0';  // don't send anything to the modem
        return;
    }
}

int modem_init()
{
    int num_bytes;
    int modemFD;
    struct termios tp;

    modemFD =
        open("/dev/serial/by-id/usb-067b_0609-if00-port0", O_RDWR | O_NOCTTY);
    if (modemFD < 0) {
        perror("failed to open modem FD");
        return -1;
    }
    // terminal settings
    tcflush(modemFD, TCIOFLUSH);
    ioctl(modemFD, TCGETS, &tp);
    cfmakeraw(&tp);
    cfsetspeed(&tp, 9600);
    ioctl(modemFD, TCSETS, &tp);

#if AT_SYNC_BAUD
    num_bytes = slow_write(modemFD, baudsync, strlen(baudsync));
    if (num_bytes <= 0) {
        perror("error sending baudsync command");
    }
#endif

#if AT_ECHO
    num_bytes = slow_write(modemFD, noecho, strlen(noecho));
    if (num_bytes <= 0) {
        perror("error sending echo disable command");
    }
#endif

    return modemFD;
}

int main()
{
    unsigned char buf[256];

    int modemFD = modem_init();

    fd_set masterfds;
    fd_set readfds;

    FD_ZERO(&masterfds);

    FD_SET(STDIN_FILENO, &masterfds);
    FD_SET(modemFD, &masterfds);

    // since STDIN_FILENO = 0
    // modemFD must be larger if it is valid
    int fdmax = modemFD;
    int num_bytes = 0;

    //setbuf(fdopen(modemFD, "r"), NULL);

    for (;;) {
        // backup master list of file descriptors because select() clears its fds param
        readfds = masterfds;

        int result = select(fdmax + 1, &readfds, NULL, NULL, NULL);
        if (result == -1) {
            perror("select() error");
            return 0;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            num_bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1);
            //printf("num_bytes: '%d'\n", num_bytes);
            if (num_bytes <= 0) {
                perror("error reading from STDIN");
                return 0;
            }
            // remove the newline
            //buf[num_bytes-1] = '\r';
            buf[num_bytes - 1] = '\0';
            //printf("> ");
            parse_command(modemFD, buf, buf);

            num_bytes = slow_write(modemFD, buf, strlen(buf));
            // when we write we only check num_bytes < 0
            // because we can may write the null byte
            // which isn't actually an error
            if (num_bytes < 0) {
                perror("error writing to modem");
                return;
            }
        }

        if (FD_ISSET(modemFD, &readfds)) {
            num_bytes = read(modemFD, buf, sizeof(buf) - 1);
            if (num_bytes <= 0) {
                perror("error reading from modemFD");
                return 0;
            }
            buf[num_bytes] = '\0';
            parse_response(buf);
        }
    }
    return 0;
}
