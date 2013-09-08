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
#include "can_names.h"

#define BUFLEN 4096
#define TYPE_ARG_IDX 0
#define ID_ARG_IDX 1
#define DATA_ARG_START_IDX 2
#define MAX_DATA_LEN 8

#define LOG_FILE_SIZE 512
#define LEN_FILE_SIZE 4

const char *gateway_address = "localhost";
int gateway_port = 1338;
static int gatewayfd;

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

        if (strlen(arg) == 2) {
            return 16 * from_hex(arg[0]) + from_hex(arg[1]);
        }

    if (strlen(arg) == 1 && ((isalpha(arg[0]) && isupper(arg[0])) || arg[0] == '.')) {
        /* otherwise maybe its a letter (eg. J P G) */
        return arg[0];
    }
    else if(strlen(arg) == 1) {
        /* otherwise maybe it's a 1-digit hex */
        return from_hex(arg[0]);
    }

    /* otherwise maybe an ASCII character wrappd in single quotes */
    if (strlen(arg) == 3 && arg[0] == '\'' && arg[2] == '\'') {
        return arg[1];
    }

    return 0xff;
}

uint8_t parse_can_arg_fail(const char *arg) {
    uint8_t parsed_arg = parse_can_arg(arg);

    if(parsed_arg == 0xff) {
        printf("Invalid argument: %s\n", arg);
        exit(1);
    }

    return parsed_arg;
}

void init_gateway_connection()
{

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
}

int ignore_can_until_type(uint8_t target_type,
                           uint8_t *type,
                           uint8_t *id,
                           uint8_t *len,
                           uint8_t data[MAX_DATA_LEN])
{
    int rc;

    while(1) {
        rc = read(gatewayfd, type, 1);
        if(rc < 1) return -1;

        rc = read(gatewayfd, id, 1);
        if(rc < 1) return -1;

        rc = read(gatewayfd, len, 1);
        if(rc < 1) return -1;

        rc = read(gatewayfd, data, *len);
        if(rc < 1) return -1;

        int i;

        // For now, only print received frames if they're
        // not the CAN type that we're expecting
        if(*type != target_type) {
            printf("Dori sent frame: %s [%x] %s [%x] %d [ ", type_names[*type], *type,
                   id_names[*id], *id, *len);

            for(i = 0; i < *len; i++) {
                printf("%x ", data[i]);
            }
            printf(" ]\n");
        }

        if(*type == target_type) {
            return 0;
        }
    }
}


void process_data_transfer(char *filename, int file_size)
{
    FILE *f;

    if(filename == NULL) {
        f = stdout;
    }
    else {
        f = fopen(filename, "w");

        if(f == NULL)
        {
            printf("Error creating file %s\n", filename);
            exit(-1);
        }
    }

    int total_recv_bytes = 0;
    uint8_t type, id, len;
    uint8_t data[MAX_DATA_LEN];

    do {
        if(ignore_can_until_type(TYPE_xfer_chunk, &type, &id, &len, data) < 0) {
            break;
        }

        // if we get a transfer chunk of length 0, that means the transfer is complete
        if(len == 0) {
            break;
        }

        total_recv_bytes += len;

        if(file_size != -1) {
            printf("received %d/%d bytes\n", total_recv_bytes, file_size);
        }

        fwrite(data, 1, len, f);
        fflush(f);
        fflush(f);
        fsync(fileno(f));
        fsync(fileno(f));

    } while(file_size == -1 || total_recv_bytes < file_size);

    if(f != stdout) {
        fclose(f);
    }

    if(total_recv_bytes == file_size) {
        printf("\nSuccessfully received '%s'\n", filename);
    }
}

// no argument version of data transfer - prints to stdout
void process_data_transfer_stdout() {
    process_data_transfer(NULL, -1);
}



void parse_can_packet_from_stdin(int num_args,
                                 char *args[],
                                 uint8_t *type,
                                 uint8_t *id,
                                 uint8_t *len,
                                 uint8_t data[MAX_DATA_LEN])
{
    uint8_t i;

    *type = parse_can_arg_fail(args[TYPE_ARG_IDX]);
    *id = parse_can_arg_fail(args[ID_ARG_IDX]);

    // subtract out type and id from length
    *len = num_args - 2;

    for(i = 0; i < *len; i++) {
        data[i] = parse_can_arg_fail(args[DATA_ARG_START_IDX + i]);
    }
}

void transmit_can_packet(uint8_t type, uint8_t id, uint8_t len, uint8_t data[MAX_DATA_LEN]) {
    uint8_t i;

    // write the type
    write(gatewayfd, &type, sizeof(type));

    // write the ID
    write(gatewayfd, &id, sizeof(id));

    // write the generated length
    write(gatewayfd, &len, sizeof(len));

    // write the data
    for(i = 0; i < len; i++) {
        write(gatewayfd, &data[i], sizeof(data[i]));
    }
}

void process_dcim_read() {
    uint8_t id, type, len;
    uint8_t data[MAX_DATA_LEN];

    uint8_t target_type = TYPE_dcim_header;

    int rc = ignore_can_until_type(target_type, &type, &id, &len, data);
    if(rc < 0) {
        printf("Error while waiting for %s\n", type_names[target_type]);
        exit(1);
    }

    uint16_t folder = data[0] | (data[1] << 8);
    printf("folder: %d\n", folder);

    uint16_t file = data[2] | (data[3] << 8);
    printf("file: %d\n", file);

    int i;
    char extension[9];

    // Read the remaining data bytes
    for(i = 0; i < len - 4; i++) {
        extension[i] = data[4 + i];
    }

    extension[i] = '\0';

    char file_path[128];
    sprintf(file_path, "DCIM/%dCANON/IMG_%d.%s", folder, file, extension);
    printf("file path: %s\n", file_path);

    target_type = TYPE_dcim_len;

    rc = ignore_can_until_type(target_type, &type, &id, &len, data);
    if(rc < 0) {
        printf("Error while waiting for %s\n", type_names[target_type]);
        exit(1);
    }

    uint32_t file_size = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    printf("file_size: %d\n", file_size);

    process_data_transfer(file_path, file_size);

    // /DCIM/113CANON/IMG_1954.JPG
    // encoded as:
    // /DCIM/[xxx]CANON/IMG_[yyyy].ZZZ
    // can message:
    // [TYPE_dcim_read] [ID_sd] [x:low] [x:high] [y:low] [y:high] [Z] [Z] [Z]
    // 71 00 D2 04 -> 113 and file 1234
    // 71 00 39 1b -> 113 and file 6969
}

void process_file_read() {
    uint8_t id, type, len;
    // add +1 so that we can add a null byte for the filename
    uint8_t data[MAX_DATA_LEN + 1];

    uint8_t target_type = TYPE_file_header;

    int rc = ignore_can_until_type(target_type, &type, &id, &len, data);
    if(rc < 0) {
        printf("Error while waiting for %s\n", type_names[target_type]);
        exit(1);
    }

    data[len] = '\0';

    char *extension = strchr((char *)data, '.') + 1;

    // file size is predefined, determined by the file extension
    // log file = 512 bytes
    // length file = 4 bytes

    uint32_t file_size = LOG_FILE_SIZE; // default size

    if(extension == NULL) {
        printf("Warning: No extension on incoming file. Using default %d bytes\n", LOG_FILE_SIZE);
    }
    else if(strcasecmp(extension, "len") == 0) {
        file_size = LEN_FILE_SIZE;
    }

    char file_path[128];
    sprintf(file_path, "files/%s", data);
    printf("file path: %s\n", file_path);

    process_data_transfer(file_path, file_size);
}

// don't need to do anything special for the file tree at the moment
void process_file_tree() {
    process_data_transfer_stdout();
}


int main(int argc, char *argv[])
{
    if(argc < 3 || argc > 11)
    {
        printf("Usage: send [D]? [TYPE] [ID] [DATA]\n");
        exit(0);
    }

    uint8_t type, id, len;
    uint8_t data[MAX_DATA_LEN];

    // If the first arg is a single D, then we connect to gateway on port 53,
    // in order to impersonate DORI
    if(strlen(argv[1]) == 1 && argv[1][0] == 'D') {
        gateway_port = 53;
        argv = &argv[1];
        argc -= 1;
    }

    init_gateway_connection();

    // skip over the executable filename
    argv = &argv[1];
    argc -= 1;

    parse_can_packet_from_stdin(argc, argv, &type, &id, &len, data);

    transmit_can_packet(type, id, len, data);

    // switch based on the type
    switch(type) {
    case TYPE_dcim_read:
        process_dcim_read();
        break;
    case TYPE_file_read:
        process_file_read();
        break;
    case TYPE_file_tree:
        process_file_tree();
        break;
    }
    return 0;
}

