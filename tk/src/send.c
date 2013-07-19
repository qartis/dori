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

// DORI debug lets us type in ascii into (netcat) to pretend to be DORI
#define DORI_DEBUG 1

const char *gateway_address = "localhost";
const int gateway_port = 1338;
static int gatewayfd;

void process_file_transfer(void *data, char *args[])
{
    command *cmd = (command *)data;
    int node_index = 0;
    int filename_index = 1;
    printf("Requesting file '%s' from node '%s'\n", args[filename_index], args[node_index]);

    char buf[128];
    sprintf(buf, "%s %d %s %s\n", cmd->name, cmd->args, args[node_index], args[filename_index]);
    int bytes = write(gatewayfd, buf, strlen(buf));

    if(bytes < 0)
    {
        perror("process_file_transfer()");
        exit(-1);
    }

    FILE *f;

    f = fopen(args[filename_index], "w");

    if(f == NULL)
    {
        printf("Error creating file %s\n", args[filename_index]);
        exit(-1);
    }

    int bytes_received = 0;
    int total_bytes = 0;
    int length_bytes_received = 0;

    do
    {
        bytes = read(gatewayfd, buf, sizeof(buf));
        if(bytes < 0)
        {
            perror("process_file_transfer()");
            exit(-1);
        }
        else if(bytes == 0)
        {
            printf("Connection to Gateway closed\n");
            break;
        }

        // 4 bytes for file length, transferred first
        if(length_bytes_received < 4)
        {
#if DORI_DEBUG
            buf[bytes] = '\0';
            total_bytes = atoi(buf);
            bytes = 0;
            length_bytes_received = 4;
#else
            int i;
            for(i = 0; i < bytes && i < 4; i++)
            {
                total_bytes += (buf[i] << (8 * (3 - length_bytes_received)));
                length_bytes_received++;
            }
            // Subtract out the bytes that we used for the file length
            bytes -= i;
#endif
        }


        if(bytes >= 0)
        {
            bytes_received += bytes;
            printf("\rReceived %d / %d bytes", bytes_received, total_bytes);
            fflush(stdout);
            fprintf(f, buf, bytes);
        }
    } while(bytes_received < total_bytes);

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


