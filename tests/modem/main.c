#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>

#include "sim900.h"

volatile uint8_t verbose;
pid_t ppid;

void ignore_signal(int ignore)
{
    printf("caught signal\n");
}

int strcasestart(const char *a, const char *b)
{
    return strncasecmp(a, b, strlen(b)) == 0;
}

int strstart(const char *a, const char *b)
{
    return strncmp(a, b, strlen(b)) == 0;
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
    printf("\n");
}

void* modem_thread(void *arg)
{
    (void)arg;
    int8_t rc;
    char c;
    char at_buf[128];
    uint8_t tcp_rx_buf[128];
    int tcp_rx_buf_len = 0;
    int count = 0;

    int tcp_toread = 0;

    for (;;) {
        rc = read(modemFD, &c, 1);
        if (rc < 0) {
            perror("read");
            exit(1);
        }

        if (verbose) {
            putchar(c);
        }

        /* if we're reading a tcp chunk */
        if (tcp_toread > 0) {
            tcp_rx_buf[tcp_rx_buf_len] = c;
            tcp_rx_buf[tcp_rx_buf_len + 1] = '\0';
            tcp_rx_buf_len++;
            tcp_toread--;
            if (tcp_toread == 0) {
                printf("Received TCP chunk: '%s'\n", tcp_rx_buf);
                hexdump(tcp_rx_buf, tcp_rx_buf_len);
                tcp_rx_buf_len = 0;
            }

            continue;
        }

        if (state == STATE_EXPECTING_PROMPT && streq(at_buf, ">")) {
            state = STATE_GOT_PROMPT;
            count = 0;
            at_buf[0] = '\0';
        } else if (c == '\r') {
            if (streq(at_buf, "OK")) {
                /* We don't care anymore */
            } else if (streq(at_buf, "ERROR")) {
                /* We don't care anymore */
            } else if (streq(at_buf, "STATE: IP INITIAL")) {
                state = STATE_IP_INITIAL;
            } else if (streq(at_buf, "STATE: IP START")) {
                state = STATE_IP_START;
            } else if (streq(at_buf, "STATE: IP GPRSACT")) {
                state = STATE_IP_GPRSACT;
            } else if (streq(at_buf, "STATE: IP STATUS")) {
                state = STATE_IP_STATUS;
            } else if (streq(at_buf, "STATE: IP PROCESSING")) {
                state = STATE_CONNECTED;
            } else if (streq(at_buf, "0, SEND OK")) {
                state = STATE_CONNECTED;
            } else if (streq(at_buf, "0, CLOSE OK")) {
                state = STATE_CLOSED;
            } else if (streq(at_buf, "0, CONNECT FAIL")) {
                state = STATE_ERROR;
            } else if (strstart(at_buf, "+CME ERROR:")) {
                state = STATE_ERROR;
            } else if (streq(at_buf, "SHUT OK")) {
                state = STATE_CLOSED;
            } else if (strcasestart(at_buf,"+RECEIVE,0,")) {
                printf("TCP: Going to read %d bytes\n", atoi(at_buf+strlen("+RECEIVE,0,")));
                tcp_toread = atoi(at_buf+strlen("+RECEIVE,0,"));

                /* modem just sent +RECEIVE,0,%d\r\n. we must chomp the \n */
                rc = read(modemFD, &c, 1);
                if (rc < 1) {
                    perror("read");
                }
            } else if (streq(at_buf, "RING")) {
                printf("ringing!\n");
                slow_write(modemFD, "ATH\r", strlen("ATH\r"));
                _delay_ms(1500);
                kill(ppid, SIGUSR1);
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
    uint8_t rc;
    int i;
    int argc;
    char **argv;
    wordexp_t p;

    wordexp(command, &p, 0);

    argv = p.we_wordv;
    argc = p.we_wordc;

    (void)argc;

    if (streq(argv[0], "connect")) {
        rc = TCPConnect();
        printf("connect: %d\n", rc);
    } else if (streq(argv[0], "disconnect")) {
        rc = TCPDisconnect();
        printf("disconnect: %d\n", rc);
    } else if (streq(argv[0], "spam")) {
        for (i = 0; i < 50; i++)
            sendATCommand("AT+CSQ");
    } else if (streq(argv[0], "send")) {
        rc = TCPSend();
        printf("send: %d\n", rc);
    } else if (strcasestart(command, "AT")) {
        verbose = 1;
        slow_write(modemFD, command, strlen(command));
        slow_write(modemFD, "\r", strlen("\r"));
        usleep(1000000);
        verbose = 0;
        printf("\n");
    } else {
        printf("error: unrecognized command: %s\n", command);
    }
}

int main(int argc, char **argv)
{
    int rc;
    char *dev;
    char buf[256];
    pthread_t thread;

    dev = "/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller-if00-port0";
    if (argc > 1)
        dev = argv[1];

    modem_init(dev);

    signal(SIGUSR1, ignore_signal);
    struct sigaction new_action;
    sigaction(SIGUSR1, NULL, &new_action);
    new_action.sa_flags &= ~SA_RESTART;
    sigaction(SIGUSR1, &new_action, NULL);


    setbuf(stdout,NULL);

    ppid = getpid();

    pthread_create(&thread, NULL, modem_thread, NULL);

    slow_write(modemFD, "ATE0\r", strlen("ATE0\r")); // turn on/off echo.
    usleep(100000);

    for (;;) {
        fprintf(stderr, "> ");
        rc = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (rc < 1) {
            if (errno == EINTR) {
                rc = TCPConnect();
                printf("remote connect: %d\n", rc);
                continue;
            } else {
                perror("read stdin");
                exit(1);
            }
        }
        buf[rc - 1] = '\0';
        parse_command(modemFD, buf);
    }

    return 0;
}
