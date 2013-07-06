#include <stdio.h>
#include <wordexp.h>
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
#include "sim900.h"

inline int strcasestart(const char *buf1, const char *buf2)
{
    return strncasecmp(buf1, buf2, strlen(buf2)) == 0;
}

int streq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

void hexdump(uint8_t *buf, int len)
{
    int i;

    for (i = 0; i < len; i++) {
        printf("0x%x ", buf[i]);
        if (((i + 1) % 16) == 0) {
            printf("\n");
        } else if (((i + 1) % 8) == 0) {
            printf(" ");
        }
    }
}

void* modem_thread(void *arg)
{
    (void)arg;
    int rc;
    char c;
    char at_buf[128];
    unsigned char tcp_rx_buf[128];
    int tcp_rx_buf_len = 0;
    int count;

    int tcp_toread = 0;

    for (;;) {
        rc = read(modemFD, &c, 1);
        if (rc < 0) {
            perror("read");
            exit(1);
        }

        /* if we're reading a tcp chunk */
        if (tcp_toread > 0) {
            tcp_rx_buf[tcp_rx_buf_len] = c;
            tcp_rx_buf_len++;
            tcp_toread--;
            if (tcp_toread == 0) {
                printf("got TCP chunk:\n");
                hexdump(tcp_rx_buf, tcp_rx_buf_len);
                tcp_rx_buf_len = 0;
            }

            continue;
        }


        received = 1;

        if (streq(at_buf, ">")) {
            flag_tcp_send = TCP_SEND_EXPECTING_DATA;
            count = 0;
            at_buf[0] = '\0';
        } else if (c == '\r') {
            if (!ignore) {
                if (streq(at_buf, "OK")) {
                    flag_ok = 1;
                } else if (streq(at_buf, "ERROR")) {
                    flag_error = 1;
                } else if (streq(at_buf, "STATE: IP INITIAL")) {
                    state = STATE_IP_INITIAL;
                } else if (streq(at_buf, "STATE: IP START")) {
                    state = STATE_IP_START;
                } else if (streq(at_buf, "STATE: IP GPRSACT")) {
                    state = STATE_IP_GPRS;
                } else if (streq(at_buf, "STATE: IP STATUS")) {
                    state = STATE_IP_STATUS;
                } else if (streq(at_buf, "STATE: TCP CONNECTING")) {
                    state = STATE_TCP_CONNECTING;
                } else if (streq(at_buf, "0, CONNECT OK")) {
                    state = STATE_TCP_CONNECTED;
                } else if (streq(at_buf, "0, SEND OK")) {
                    state = STATE_TCP_SEND_OK;
                } else if (streq(at_buf, "0, CONNECT FAIL")) {
                    state = STATE_TCP_CONNECT_FAILED;
                } else if (streq(at_buf, "SHUT OK")) {
                    state = STATE_CLOSED;
                } else if (strcasestart(at_buf,"+RECEIVE,0,")) {
                    printf("TCP: Going to read %d bytes", atoi(at_buf+strlen("+RECEIVE,0,")));
                    tcp_toread = atoi(at_buf+strlen("+RECEIVE,0,"));
                } else if (streq(at_buf, "RING")) {
                    printf("ringing!\n");
                    slow_write(modemFD, "ATH\r", strlen("ATH\r"));
                }
            } 

            count = 0;
            at_buf[0] = '\0';
        } else if (c != '\n' && c != '\0') {
            at_buf[count] = c;
            at_buf[count + 1] = '\0';
            count++;
        }
    }
}

void parse_command(int modemFD, char *command)
{
    int argc;
    char **argv;
    wordexp_t p;

    wordexp(command, &p, 0);

    argv = p.we_wordv;
    argc = p.we_wordc;

    if (strcmp(argv[0], "connect") == 0){
        if (argc != 1) {
            printf("usage: connect\n");
            return;
        }

        TCPConnect("h.qartis.com", 53);
    } else if (strcmp(argv[0], "spam") == 0){
        int i;
        for (i = 0; i < 50; i++)
            sendATCommand("AT+CSQ");
    } else if (strcasecmp(argv[0], "send") == 0) {
        if (argc != 1) {
            printf("usage: send [chars] (no spaces allowed)\n");
            return;
        }
        if (!flag_tcp_state == TCP_CONNECTED) {
            printf("not connected!\n");
            return;
        }

        printf("connected!\n");
        printf("arg1: %s\n",argv[1]);
        TCPSend();

    }else if (strcasecmp(argv[0], "close") == 0) {
        TCPClose();
    }else if (strcasestart(command, "AT")) {
        slow_write(modemFD, command, strlen(command));
        slow_write(modemFD, "\r", strlen("\r"));
        usleep(1000000);
        if (flag_error) {
            printf("got 'error'\n");
        } else if (flag_ok) {
            printf("got 'ok'\n");
        } else {
            printf("wtf!\n");
            exit(1);
        }
    //} else if (strcmp(argv[0], "") == 0) { // don't whine if I press enter
    } else {
        printf("error: unrecognized command: %s\n", command);
    }
}



int main(int argc, char * argv[])
{
    char buf[256];
    int rc;
    pthread_t thread;
    char *dev;

    dev = "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller-if00-port0";
    if (argc > 1)
        dev = argv[1];

    modem_init(dev);

    setbuf(stdout,NULL);

    pthread_create(&thread, NULL, modem_thread, NULL);

    ignore = 1;
    slow_write(modemFD, "ATE0\r", strlen("ATE0\r")); // turn on/off echo.
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
