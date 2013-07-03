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

uint8_t TCP_state_is(int state){
	uint16_t retry = TIMEOUT_RETRIES;
	while (retry-- && flag_tcp_state != state && !flag_error){
  		_delay_ms(20);
  }
  return flag_tcp_state == state;
}
uint8_t IP_state_is(int state){
	uint16_t retry = TIMEOUT_RETRIES;
	while (retry-- && flag_ip_state != state && !flag_error){
  		_delay_ms(20);
  }
  return flag_ip_state == state;
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
	flag_error = 0;
	sendATCommand(CLOSE_TCP_CMD);
  if (!TCP_state_is(TCP_CLOSED)){
  	printf("error closing tcp.\n");
  }
  
  flag_error = 0;
	sendATCommand(SHUT_CMD);
  if (!IP_state_is(IP_CLOSED)){
  	printf("error shutting off radio. :/\n");
  }
}

void TCPSend(char * buf, int length)
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
  while  (ch !='.') {
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

// reads the flags that are sent by the responses... although it's not 100% perfect because sometimes the modem returns slightly different answers if it's already been partially connected... we should look into whether there are a few extra commands that need to go into TCPclose in order to really turn the radio off.
int TCPConnect(const char * host, uint16_t port)
{
	flag_error = 0;
	char buf[128];
	if (flag_tcp_state != TCP_CLOSED || flag_ip_state != IP_CLOSED){
		printf("Connection is open. Closing connection in order to reconnect.\n");
		TCPClose();
	}
	// Selects Single-connection mode
	sendATCommand(SINGLE_CON_CMD);
	// wait for initial IP status...
	sendATCommand(STATUS_CMD);
	if(!IP_state_is(IP_INITIAL)){
		printf("error setting mux mode.\n");
		//return 1;
	}
	sendATCommand(PAP_SETUP_CMD);
	flag_error = 0;
	sendATCommand(STATUS_CMD);
	if(!IP_state_is(IP_START)){
		printf("error setting pap settings.\n");
		//return 1;
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
