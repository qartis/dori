#include <stdio.h>
#include <string.h>
#include <ctype.h>
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
#include "can.h"

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

        fwrite(buf, 1, bytes, f);
        fflush(f);
        fflush(f);
        fsync(fileno(f));
        fsync(fileno(f));

    } while(total_recv_bytes < file_size);


    fclose(f);
    printf("\nSuccessfully received '%s' from '%s'\n", args[filename_index], args[node_index]);
}

void process_dcim_transfer_request(void *data, char *args[])
{
    (void)data;
    write(gatewayfd, "IMG 1 abc", strlen("IMG 1 abc"));

    FILE *f;

    f = fopen("picture.jpg", "w");

    if(f == NULL)
    {
        printf("Error creating dcim %s\n", "picture.jpg");
        exit(-1);
    }

    int total_recv_bytes = 0;
    int file_size = 5790;

    int bytes = 0;
    unsigned buf[BUFLEN];

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

        fwrite(buf, 1, bytes, f);
        fflush(f);
        fflush(f);
        fsync(fileno(f));
        fsync(fileno(f));

    } while(total_recv_bytes < file_size);


    fclose(f);
    printf("\nSuccessfully received dcim 'picture.jpg' from DORI\n");

}

uint8_t parse_can_arg(const char *arg)
{
    /* first check all the type names */
#define X(name, value) if (strequal(arg, #name)) return TYPE_ ##name; \

    TYPE_LIST(X)
#undef X

    /* then try all the id names */
#define X(name, value) if (strequal(arg, #name)) return ID_ ##name; \

    ID_LIST(X)
#undef X
    /* otherwise maybe it's a 2-digit hex */
    if (strlen(arg) == 2) {
        return 16 * from_hex(arg[0]) + from_hex(arg[1]);
    }

    /* otherwise maybe it's a 1-digit hex */
    if (strlen(arg) == 1) {
        return from_hex(arg[0]);
    }

    printf("WTF! given arg '%s'\n", arg);
    return 0xff;
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

    int command_index = 1;

    // If the first argument is not a valid command
    // then we'll assume its the start of a CAN frame
    printf("command: %s\n", argv[command_index]);
    if(!has_command(argv[command_index]))
    {
        int i;
        // subtract 1 for the executable filename
        int true_argc = argc - 1;

        if(true_argc < 3) {
            printf("Invalid number of arguments for CAN frame\n");
            exit(1);
        }

        // CAN frame: type, id, data length, data
        for(i = 0; i < true_argc; i++) {
            uint8_t arg = parse_can_arg(argv[command_index + i]);
            write(gatewayfd, &arg, sizeof(arg));
        }
    }
    else
    {
        // subtract 2
        // 1 for executable filename
        // 1 for command name
        int true_argc = argc - 2;

        if(run_command(argv[command_index], true_argc, &argv[2]) < 0)
        {

            printf("Failed to process command: %s\n", argv[1]);
            exit(-1);
        }
    }

    return 0;
}


