#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>

#include "sim900.h"

int modemFD;

volatile enum state state;

uint8_t wait_for_state(uint8_t goal_state)
{
    uint8_t retry;

    /* 255 * 10 = 2550 ms */
    retry = 255;
    while (state != goal_state && state != STATE_ERROR && --retry)
        _delay_ms(20);

    return state != goal_state;
}


size_t slow_write(int fd, const char *buf, size_t count)
{
    unsigned i = 0;
    size_t rc;

    while (i < count) {
        rc = write(fd, (uint8_t *)buf + i, 1);
        if (rc < 1) {
            perror("write");
            exit(1);
        }
        usleep(5000); // was set to 5000 for file transfer test

        if (buf[i] == '\r') {
            usleep(10000);
        }
        i++;
    }

    return i;
}

void _delay_ms(int delay)
{
    usleep(delay*1000);
}

/*
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
        perror("file not found/inaccessible.");
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
*/


// TODO: optimize this?
void sendATCommand(const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    slow_write(modemFD, buf, strlen(buf));
    slow_write(modemFD, "\r", strlen("\r"));

    _delay_ms(100);
}


uint8_t TCPDisconnect(void)
{
    uint8_t rc;

    state = STATE_UNKNOWN;

    sendATCommand("AT+CIPCLOSE=0");
    wait_for_state(STATE_CLOSED);

    state = STATE_UNKNOWN;

    sendATCommand("AT+CIPSHUT");
    wait_for_state(STATE_CLOSED);

    sendATCommand("AT+CIPSTATUS");
    rc = wait_for_state(STATE_IP_INITIAL);

    return rc;
}

uint8_t TCPSend(void)
{
    char c;
    int len;
    uint8_t rc;

    fprintf(stderr, "Data len: ");

    scanf("%d", &len);

    /* chomp newline */
    (void)getchar();

    state = STATE_EXPECTING_PROMPT;
    sendATCommand("AT+CIPSEND=0,%d", len);

    rc = wait_for_state(STATE_GOT_PROMPT);
    if (rc != 0) {
        printf("wanted to send data, but didn't get prompt\n");
        /* send CAN error, but continue regardless */

        return 1;
    }
  
    fprintf(stderr, "Data to send: ");
    while (len--) {
        c = getchar();
        write(modemFD, &c, 1);
    }

    rc = wait_for_state(STATE_CONNECTED);
    if (rc != 0) {
        printf("Sending failed\n");
    } else {
        printf("Sending succeeded\n");
    }

    return rc;
}

uint8_t TCPConnect(void)
{
    uint8_t rc;

    TCPDisconnect();

    /* Multi-connection mode in order to encapsulate received data */
    sendATCommand("AT+CIPMUX=1");

    sendATCommand("AT+CIPSTATUS");
    rc = wait_for_state(STATE_IP_INITIAL);
    if (rc != 0) {
        printf("failed to get IP state INITIAL\n");
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf("state: IP INITIAL\n");

    sendATCommand("AT+CSTT=\"goam.com\",\"wapuser1\",\"wap\"");

    sendATCommand("AT+CIPSTATUS");
    rc = wait_for_state(STATE_IP_START);
    if (rc != 0) {
        printf("failed to get IP state START\n");
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf("state: IP START\n");

    /* This command takes a little while */
    sendATCommand("AT+CIICR");
    _delay_ms(2000);

    sendATCommand("AT+CIPSTATUS");
    rc = wait_for_state(STATE_IP_GPRSACT);
    if (rc != 0) {
        printf("Failed to get IP state GPRSACT\n");
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf("state: IP GPRSACT\n");

    /* We aren't online until we check our IP address */
    sendATCommand("AT+CIFSR");

    sendATCommand("AT+CIPSTATUS");
    rc = wait_for_state(STATE_IP_STATUS);
    if (rc != 0) {
        printf("Failed to get IP state STATUS\n");
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf("state: IP STATUS\n");

    sendATCommand("AT+CIPSTART=0,\"TCP\",\"h.qartis.com\",\"53\"");   

    sendATCommand("AT+CIPSTATUS");
    rc = wait_for_state(STATE_CONNECTED);
    if (rc != 0) {
        printf("Failed to get IP state IP PROCESSING (connected)\n");
        /* dispatch a MODEM_ERROR CAN packet and try to continue */
        return rc;
    }

    printf("state: IP PROCESSING (connected)\n");

    return 0; 
}


int modem_init(const char *device)
{
    modemFD = open(device, O_RDWR | O_NOCTTY);
    if (modemFD < 0) {
        perror("open modemFD");
        exit(1);
    }

    /*
    struct termios tp;

    tcflush(modemFD, TCIOFLUSH);
    ioctl(modemFD, TCGETS, &tp);
    cfmakeraw(&tp);
    cfsetspeed(&tp, 9600);
    ioctl(modemFD, TCSETS, &tp);
    */

    return modemFD;
}
