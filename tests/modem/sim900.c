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
#include "sim900.h"

int modemFD;

volatile int flag_ok;
volatile int flag_error;
volatile int flag_nocarrier;
volatile int flag_connect;
volatile int flag_tcp_state;
volatile int flag_tcp_send;
volatile int flag_tcp_received;
volatile int ip_state;
volatile int flag_http;
volatile int received;
volatile int ignore;

size_t slow_write(int fd, const char *buf, size_t count)
{
    unsigned i = 0;
    size_t rc;

    flag_ok = 0;
    flag_error = 0;

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

void _delay_ms(int delay){
	usleep(delay*1000);
}

uint8_t get_ip_state(int state){
    uint16_t retry;
    
    ip_state = STATE_UNKNOWN;

    sendATCommand(STATUS_CMD);

    retry = TIMEOUT_RETRIES;
    while (ip_state == STATE_UNKNOWN && --retry)
        _delay_ms(20);

    return ip_state;
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
void sendATCommand(const char* command)
{
				flag_ok = 0;
				flag_error = 0;
				slow_write(modemFD, command, strlen(command));
				slow_write(modemFD, "\r", strlen("\r"));
				_delay_ms(100);
}


void TCPClose(void)
{
	sendATCommand(CLOSE_TCP_CMD);
	sendATCommand(SHUT_CMD);
}

void TCPSend(void)
{
	flag_error = 0;
	flag_tcp_send = 0;
	char send_cmd[50];
 	sprintf(send_cmd, "AT+CIPSEND=0\r");
  slow_write(modemFD, send_cmd, strlen(send_cmd));
  uint16_t retry = TIMEOUT_RETRIES;
  while (retry-- && flag_tcp_send != TCP_SEND_EXPECTING_DATA  && !flag_error){
  		_delay_ms(20);
  }
  if (flag_tcp_send != TCP_SEND_EXPECTING_DATA || flag_error){
  	printf("error sending.\n"); 
  	// need to figure out how to properly cancel the send.
  	return;
  }
  
	printf("ok to send.\n"); 
  //_delay_ms(3000);
  flag_error = 0;
  flag_ok = 0;
  char ch = 0;
  while (ch !='.') {
        ch = getchar();
        if (ch == '.')
        	slow_write(modemFD, "\x1a", strlen("\x1a"));
        else 
        	write(modemFD, &ch, 1);
    }
 // slow_write(modemFD, buf, length); 
 // slow_write(modemFD, "\x1a", strlen("\x1a"));
  retry = TIMEOUT_RETRIES;
  while (retry-- && flag_tcp_send!=TCP_SEND_OK  && !flag_error){
  		_delay_ms(20);
  }
  if (flag_tcp_send!=TCP_SEND_OK)
  	printf("Sending failed.\n");
  else
  	printf("Sending succeeded.\n");
}

uint8_t wait_for_ip_status(uint8_t goal_state)
{
    uint8_t retry;

	sendATCommand("AT+CIPSTATUS");

    retry = 255;
    while (state != goal_state && --retry)
        _delay_ms(20);

    return state != goal_state;
}

// reads the flags that are sent by the responses... although it's not 100% perfect because sometimes the modem returns slightly different answers if it's already been partially connected... we should look into whether there are a few extra commands that need to go into TCPclose in order to really turn the radio off.
uint8_t TCPConnect(const char * host, uint16_t port)
{
	flag_error = 0;
	char buf[128];
    uint8_t retry;

#define STATUS_CMD "AT+CIPSTATUS"
#define SINGLE_CON_CMD "AT+CIPMUX=1"
#define PAP_SETUP_CMD "AT+CSTT=\"goam.com\",\"wapuser1\",\"wap\""
#define CLOSE_TCP_CMD "AT+CIPCLOSE=0"
#define SHUT_CMD "AT+CIPSHUT"
#define SEND_CMD "AT+CIPSEND=0\r"
#define WIRELESS_UP_CMD "AT+CIICR"
#define GET_IP_ADDR_CMD "AT+CIFSR"
#define CONNECT_CMD "AT+CIPSTART=0,\"TCP\",\"%s\",\"%d\"\r"

    TCPClose();

	/* Multi-connection mode in order to encapsulate received data */
	sendATCommand("AT+CIPMUX=1");

    /* At this point we should be in "IP INITIAL" state */
	sendATCommand("AT+CIPSTATUS");

    retry = 255;
    while (state != STATE_IP_INITIAL && --retry)
        _delay_ms(20);

    if (state != STATE_IP_INITIAL) {
        printf("Failed to get initial IP status\n");

        /* we should probably dispatch a MODEM_ERROR CAN packet
           and then continue with the IP setup */

        return 1;
    }

    /* Sending PAP credentials also moves us to "IP START" */
	sendATCommand("AT+CSTT=\"goam.com\",\"wapuser1\",\"wap\"");

    rc = wait_for_ip_status(STATUS_IP_INITIAL);
    if (rc != 0) {
        printf("Failed to get initial IP status\n");
        return rc;
    }

	sendATCommand(WIRELESS_UP_CMD);
	_delay_ms(3000);
	flag_error = 0;
	sendATCommand(STATUS_CMD);
	if(!IP_state_is(IP_GPRS)){
		printf("error connecting to gprs.\n");
		//return 1;
	}
	sendATCommand(GET_IP_ADDR_CMD);
	sendATCommand(STATUS_CMD);
	if(!IP_state_is(IP_STATUS)){
		printf("error opening IP.\n");
		//return 1;
	}
	//sendATCommand("AT+CIPSTART=0,\"TCP\",\"h.qartis.com\",\"53\"");   
	sprintf(buf,CONNECT_CMD, host, port); 
  slow_write(modemFD,buf, strlen(buf)); 
  if(!TCP_state_is(TCP_CONNECTED)){
		printf("error opening TCP Connection.\n");
		return 1;
	}
	else
		printf("TCP Connected.\n");  
  return 0; 
}


int modem_init(const char *device)
{
    struct termios tp;

    modemFD = open(device,O_RDWR | O_NOCTTY);
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
