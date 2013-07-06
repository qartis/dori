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

#define STATUS_CMD "AT+CIPSTATUS"
#define SINGLE_CON_CMD "AT+CIPMUX=1"
#define PAP_SETUP_CMD "AT+CSTT=\"goam.com\",\"wapuser1\",\"wap\""
#define CLOSE_TCP_CMD "AT+CIPCLOSE=0"
#define SHUT_CMD "AT+CIPSHUT"
#define SEND_CMD "AT+CIPSEND=0\r"
#define WIRELESS_UP_CMD "AT+CIICR"
#define GET_IP_ADDR_CMD "AT+CIFSR"
#define CONNECT_CMD "AT+CIPSTART=0,\"TCP\",\"%s\",\"%d\"\r"

#define TIMEOUT_RETRIES 500

#define BUFFER_SIZE 256
#define AT_NO_ECHO 0
#define AT_SYNC_BAUD 0

#define IP_CLOSED			0
#define IP_INITIAL	1
#define IP_START		2
#define IP_GPRS			3
#define IP_STATUS		4

#define TCP_CLOSED				0
#define TCP_CONNECTING		1
#define TCP_CONNECTED			2

#define TCP_CONNECT_FAILED		3

#define TCP_SEND_OK							1
#define TCP_SEND_EXPECTING_DATA 2
#define TCP_SEND_FAILED					0

extern int modemFD;

extern volatile int flag_ok;
extern volatile int flag_error;
extern volatile int flag_nocarrier;
extern volatile int flag_connect;
extern volatile int flag_tcp_state;
extern volatile int flag_tcp_send;
extern volatile int flag_tcp_received;
extern volatile int flag_ip_state;
extern volatile int flag_http;
extern volatile int received;
extern volatile int ignore;

void sendATCommand(const char* command);
void TCPSend(void);
int TCPConnect(const char * host, uint16_t port);
void TCPClose(void);
void upload(const char *filename);

int modem_init(const char *device);

size_t slow_write(int fd, const char *buf, size_t count);
void _delay_ms(int delay);