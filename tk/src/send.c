#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <time.h>
#include <signal.h>
#include "command.h"

#define BUFLEN 4096

const char *gateway_address = "localhost";
const int gateway_port = 1338;
static int gatewayfd;

int strequal(const char *a, const char *b)
{
    return !strcmp(a, b);
}

void process_file_transfer(void *data, char *argv[])
{
    (void)data;
    int command_index = 1;
    int node_index = 2;
    int filename_index = 3;
    printf("Requesting file '%s' from node '%s'\n", argv[filename_index], argv[node_index]);

    char buf[128];
    sprintf(buf, "%s %s %s\n", argv[command_index], argv[node_index], argv[filename_index]);
    send(gatewayfd, buf, strlen(buf), 0);

    // TODO: save gateway's 
}

int process_command(int argc, char *argv[])
{
    // GET [NODE ID] [BUFFER INDEX]
    int i;
    int command_index = 1;

    for(i = 0; i < total_shell_commands; i++)
    {
        if(strequal(argv[command_index], commands[i].name))
        {
            // Subtract 2 here:
            // 1 for the command (aka GET)
            // 1 for first command line arg (executable filename)
            if(commands[i].args == argc - 2)
            {
                commands[i].func(&commands[i], argv);
                return 1;
            }
            else {
                printf("Invalid number of arguments for command %s. Expected %d, got %d.\n", argv[command_index], commands[i].args, argc - 1);
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("Usage: send GET [node id] [buffer number]\n");
        exit(0);
    }

    struct sockaddr_in serv_addr;
    struct hostent *server = NULL;

    gatewayfd = socket(AF_INET, SOCK_STREAM, 0);
    if(gatewayfd < 0)
    {
        printf("error opening socket\n");
        exit(-1);
    }

    server = gethostbyname(gateway_address);
    if(server == NULL)
    {
        printf("error, no such host\n");
        exit(-1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(gateway_port);

    if (connect(gatewayfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    {
        printf("Failed to connect to Gateway\n");
        exit(-1);
    }

    /*
       1) Shell initiates a file transfer request ->  2) Shell blocks while it waits for the content to be received
       */

    if(!process_command(argc, argv))
    {
        printf("Failed to process command: %s\n", argv[1]);
        exit(-1);
    }

    return 0;
}


