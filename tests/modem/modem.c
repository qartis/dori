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

	 // printf("%x %c\n", c, isprint(c) ? c : '?');
		  putchar(c);

		  received = 1;

		  if (c == '\r') {
		      if (!ignore) {
		          if (streq(buf, "OK")) {
		              flag_ok = 1;
		          } else if (streq(buf, "ERROR")) {
		              flag_error = 1;
		          } else if (streq(buf, "STATE: IP INITIAL")) {
		              flag_ip_state = IP_INITIAL;
		          }else if (streq(buf, "STATE: IP START")) {
		              flag_ip_state = IP_START;
		          }else if (streq(buf, "STATE: IP GPRSACT")) {
		              flag_ip_state = IP_GPRS;
		          }else if (streq(buf, "STATE: IP STATUS")) {
		              flag_ip_state = IP_STATUS;
		          }else if (streq(buf, "STATE: TCP CONNECTING")) {
		              flag_tcp_state = TCP_CONNECTING;
		          }else if (streq(buf, "CONNECT OK")) {
		              flag_tcp_state = TCP_CONNECTED;
		          }else if (streq(buf, "SEND OK")) {
		              flag_tcp_send = TCP_SEND_OK;
		          }else if (strcasestart(buf, ">")) {
		              flag_tcp_send = TCP_SEND_EXPECTING_DATA;
		          }else if (streq(buf, "CONNECT FAIL")) {
		              flag_tcp_state = TCP_CONNECT_FAILED;
		          }else if (streq(buf, "CLOSE OK")) {
		              flag_tcp_state = TCP_CLOSED;
		          }else if (streq(buf, "SHUT OK")) {
		              flag_ip_state = IP_CLOSED;
		              flag_tcp_state = TCP_CLOSED;
		          }else if (strncmp(buf, "HTTP", 4) == 0) {
		              char *status = buf + strlen("HTTP/1.0 ");
		              flag_http = atoi(status);
		          } else if (strcasestart(buf, "Content-length")) {
		              char *length = buf + strlen("Content-length: ");
		              if (flag_http) {
		                  http_len = atoi(length);
		              }
		          } else if (flag_http > 0 && http_len > 0 && count == 0) {
		              /* get rid of last \n from http header */
		              read(modemFD, &c, 1);
		              int i;
		              for (i = 0; i < http_len; i++) {
		                  read(modemFD, &c, 1);
		              }

		              flag_http = 0;
		          } else if (count > 0) {
		            printf("received: '%s'\n", buf);
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

void parse_command(int modemFD, char *command)
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

   /* ignore = 1;
    slow_write(modemFD, "AT\r", 3);
    usleep(1000000);
    ignore = 0;
		*/
    if (strcmp (cmd, "connect")==0){
    		if (num_args != 2) {
            printf("usage: connect [hostname] [port]\n");
            return;
        }	
        TCPConnect(arg1, atoi(arg2));
    } else if (strcasecmp(cmd, "put") == 0) {
        if (num_args != 1) {
            printf("usage: put [filename]\n");
            return;
        }
        if (!flag_ok || flag_error || flag_nocarrier || flag_connect) {
            printf("not connected!\n");
            return;
        }

        printf("connected!\n");

        upload(arg1);

        uint16_t retry = 255;
        while (flag_http == 0 && --retry) {
            usleep(100000);
        }

        if (retry == 0) {
            printf("no response!\n");
        } else if (flag_http == 200) {
            printf("upload success\n");
        } else {
            printf("http error: %d\n", flag_http);
        }
    } else if (strcasecmp(cmd, "send") == 0) {
        if (num_args != 1) {
            printf("usage: send [chars] (no spaces allowed)\n");
            return;
        }
        if (!flag_tcp_state == TCP_CONNECTED) {
            printf("not connected!\n");
            return;
        }

        printf("connected!\n");

        TCPSend(arg1, strlen(arg1));

    }else if (strcasecmp(cmd, "close") == 0) {
        TCPClose();
    }else if (strcasestart(command, "AT")) {
        command[length] = '\r';
        command[length + 1] = '\0';
        slow_write(modemFD, command, strlen(command));
        usleep(1000000);
        if (flag_error) {
            printf("got 'error'\n");
        } else if (flag_ok) {
            printf("got 'ok'\n");
        } else {
            printf("wtf!\n");
            exit(1);
        }
    }else if (strcmp(cmd, "") == 0) { // don't whine if I press enter
    } else {
        printf("error: unrecognized command: %s\n", cmd);
    }
}

    

int main(int argc, char * argv[])
{
    char buf[256];
    int rc;
    if (argc >1)
        modem_init(argv[1]);
    else
        modem_init("/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller-if00-port0");

    pthread_t thread;

    pthread_create(&thread, NULL, interrupt, NULL);

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
