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

void process_file_transfer_request(void *data, char *args[])
{
    command *cmd = (command *)data;
    int node_index = 0;
    int filename_index = 1;
    printf("Requesting file '%s' from node '%s'\n", args[filename_index], args[node_index]);

    char buf[BUFLEN];
    sprintf(buf, "%s %d %s %s\n", cmd->name, cmd->args, args[node_index], args[filename_index]);
    int bytes = write(gatewayfd, buf, strlen(buf));

    if(bytes < 0)
    {
        perror("process_file_transfer_request()");
        exit(-1);
    }

    // hardcode file size to 100 for now
    int file_size = 100;

    /*
    // Read the length of the file, if its -1 then there was an error
    read(gatewayfd, &file_size, sizeof(file_size));
    */

    if(file_size < 0) {
        printf("Error receiving file from DORI\n");
        exit(-1);
    }

    FILE *f;

    f = fopen(args[filename_index], "w");

    if(f == NULL)
    {
        printf("Error creating file %s\n", args[filename_index]);
        exit(-1);
    }

    int total_recv_bytes = 0;

    do {
        bytes = read(gatewayfd, buf, sizeof(buf));
        if(bytes < 0)
        {
            perror("process_file_transfer()");
            exit(-1);
        }
        else if(bytes == 0)
        {
            printf("Connection to Gateway closed\n");
            exit(-1);
        }

        total_recv_bytes += bytes;

        printf("received %d/%d bytes\n", total_recv_bytes, file_size);

        fprintf(f, buf, bytes);

    } while(total_recv_bytes < file_size);


    fclose(f);
    printf("\nSuccessfully received '%s' from '%s'\n", args[filename_index], args[node_index]);
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

    // subtract 2
    // 1 for first binary filename
    // 1 for command name
    int true_argc = argc - 2;
    int command_index = 1;

    if(run_command(argv[command_index], true_argc, &argv[2]) < 0)
    {
        printf("Failed to process command: %s\n", argv[1]);
        exit(-1);
    }

    return 0;
}


