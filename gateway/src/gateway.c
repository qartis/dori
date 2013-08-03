#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <time.h>
#include "can.h"
#include "command.h"
#include <signal.h>
#include <ctype.h>

#define MAX 1024
#define DORIPORT 53
#define TKPORT 1337
#define SHELLPORT 1338
#define BUFLEN 4096

#define CAN_TYPE_IDX 0
#define CAN_ID_IDX 1
#define CAN_LEN_IDX 2
#define CAN_HEADER_LEN 3

#define LOGFILE_SIZE 100

sqlite3 *db;
fd_set master;

typedef enum {
    DORI,
    TK,
    SHELL,
} client_type;

typedef struct {
    int fd;
    client_type type;
} client;


/* CAN message
   typedef struct {
   unsigned char type;
   unsigned char id;
   unsigned char data[8];
   } dori_msg;
   */

char *msg_ids[] = { "any", "ping", "pong", "laser", "gps", "temp", "time", "log", "invalid" };

char timestamp[128];
static client clients[128];
static int nclients;
static int dorifd;
static int tkfd;
static int shellfd;
static int siteid;

static char doribuf[BUFLEN];

static int doribuf_len;

typedef enum {
    DISCONNECTED,
    CONNECTED,
    FILE_TRANSFER
} shell_state;


static shell_state cur_shell_state = DISCONNECTED;
static client *active_shell_client;
static client *active_dori_client;

void error(const char *str)
{
    perror(str);
    exit(1);
}

void dberror(sqlite3 *db)
{
    printf("db error: %s\n", sqlite3_errmsg(db));
}

int strprefix(const char *a, const char *b)
{
    return !strncmp(a, b, strlen(b));
}

client* find_client(int fd)
{
    client* c = NULL;
    int i;

    for(i = 0; i < nclients; i++)
    {
        if(clients[i].fd == fd)
        {
            return &clients[i];
        }
    }

    return c;
}

void remove_client(int fd)
{
    int i;

    close(fd);

    for (i = 0; i < nclients; i++)
    {
        if (clients[i].fd == fd)
        {
            memmove(&clients[i], &clients[i + 1],
                    (nclients - i - 1) * sizeof(clients[0]));
            nclients--;
            i--;
            break;
        }
    }

    FD_CLR(fd, &master);
}

ssize_t safe_write(int fd, const char *buf, size_t count)
{
    ssize_t rc;
    size_t wroteb;

    wroteb = 0;
    while (wroteb < count)
    {
        rc = send(fd, buf + wroteb, count - wroteb, MSG_NOSIGNAL);
        if (rc < 1)
        {
            remove_client(fd);
            printf("err 1: client disconnected during write\n");
            break;
        }

        wroteb += rc;
    }

    return wroteb;
}

int printfd(int fd, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    safe_write(fd, buf, strlen(buf));

    return strlen(buf);
}

int sqlite_cb(void *arg, int ncols, char **cols, char **rows)
{
    (void)rows;

    int *fd = arg;
    int i;

    for (i = 0; i < ncols; i++)
    {
        printfd(*fd, "%s", cols[i]);
        if(send(*fd, "", 1, MSG_NOSIGNAL) < 1)
        {
            printf("err 2: client disconnected during write\n");
            remove_client(*fd);
            return -1;
        }
    }

    return 0;
}

int site_cb(void *arg, int ncols, char **cols, char **rows)
{
    (void)rows;
    (void)arg;
    if(ncols == 1 && cols[0] != '\0')
    {
        siteid = atoi(cols[0]);
    }
    return 0;
}


void exec_query(char *query)
{
    int rc = sqlite3_exec(db, query, NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        printf("sqlite error: %s\n", sqlite3_errmsg(db));
    }
}


void process_dori_bytes(char *buf, int len)
{
    memcpy(&doribuf[doribuf_len], buf, len);
    doribuf_len += len;

    while(doribuf_len > CAN_HEADER_LEN) {
        // Extract the payload size from the CAN header
        uint8_t datalen = doribuf[CAN_LEN_IDX];

        if(doribuf_len >= CAN_HEADER_LEN + datalen) {
            int i;
            uint8_t type = doribuf[CAN_TYPE_IDX];

            uint8_t id  = doribuf[CAN_ID_IDX];
            char *data = malloc(sizeof(unsigned) * datalen);
            memset(data, '\0', datalen);

            for(i = 0; i < datalen; i++) {
                data[datalen - 1 - i] = doribuf[CAN_HEADER_LEN + i];
            }

            switch(type) {
            case TYPE_file_header:
                {
                    int file_size = LOGFILE_SIZE;
                    if(active_shell_client) {
                        write(active_shell_client->fd, &file_size, sizeof(file_size));
                    }
                    break;
                }
            case TYPE_xfer_chunk:
                printf("got a file chunk\n");
                if(active_shell_client) {
                    write(active_shell_client->fd, data, datalen);
                    // don't need the CTS
                    //write(active_dori_client->fd, "b\n", strlen("b\n"));
                }
                break;
            case TYPE_file_error:
                {
                    int file_size = -1;
                    printf("got a file error from ID %.2x\n", id);
                    if(active_shell_client) {
                        write(active_shell_client->fd, &file_size, sizeof(file_size));
                    }

                    break;
                }
            default:
                break;
            }

            doribuf_len -= (CAN_HEADER_LEN + datalen);

            memmove(doribuf, &doribuf[CAN_HEADER_LEN + datalen],
                    (doribuf_len) * sizeof(char));

            free(data);
        }
    }
}

void process_file_transfer_request(void *data, char *args[])
{
    (void)data;
    int node_index = 0;
    int filename_index = 1;
    printf("Requesting file '%s' from node '%s'\n",  args[filename_index], args[node_index]);

    cur_shell_state = FILE_TRANSFER;

    // ignoring node id for now

    if(active_dori_client) {
        write(active_dori_client->fd, "a", strlen("a"));
        write(active_dori_client->fd, args[filename_index], strlen(args[filename_index]));
        write(active_dori_client->fd, "\n", strlen("\n"));
    }

}

int process_shell_msg(client *c, char *msg)
{
    (void)c;
    printf("msg: %s\n", msg);
    char buf[128];
    strcpy(buf, msg);

    char *cmd = strtok(buf, " ");
    if(cmd == NULL)
    {
        return -1;
    }

    char *num_args_str = strtok(NULL, " ");
    if(num_args_str == NULL)
    {
        return -1;
    }

    int num_args = atoi(num_args_str);
    if(num_args < 0)
    {
        return -1;
    }

    // allocate the args we need
    char **args = malloc(num_args);
    int i;
    for(i = 0; i < num_args; i++)
    {
        char *arg = strtok(NULL, " ");
        if(arg == NULL)
        {
            break;
        }
        args[i] = strdup(arg);
    }

    // take the actual number of arguments we parsed out
    int argc = i;
    int result = run_command(cmd, argc, args);

    // Free as many args as we allocated
    int j;
    for(j = 0; j < argc; j++)
    {
        free(args[j]);
    }
    free(args);

    return result;
}

int main()
{
    int newfd, maxfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    socklen_t len;
    fd_set readfds;
    int optval, rc, fd;
    char buf[BUFLEN];

    sqlite3_open("data/db", &db);

    if(db)
    {
        sqlite3_exec(db, "select max(site) from records;", site_cb, NULL, NULL);
    }


    dorifd = socket(AF_INET, SOCK_STREAM, 0);
    if (dorifd == -1)
        error("dori socket");

    shellfd = socket(AF_INET, SOCK_STREAM, 0);
    if (shellfd == -1)
        error("shell socket");

    tkfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tkfd == -1)
        error("tk socket");

    optval = 1;
    setsockopt(dorifd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    setsockopt(shellfd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    setsockopt(tkfd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    memset(&servaddr, '\0', sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    servaddr.sin_port = htons(DORIPORT);
    rc = bind(dorifd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (rc == -1)
        error("bind: dorifd");

    servaddr.sin_port = htons(SHELLPORT);
    rc = bind(shellfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (rc == -1)
        error("bind: shellfd");

    servaddr.sin_port = htons(TKPORT);
    rc = bind(tkfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (rc == -1)
        error("bind: tkfd");

    rc = listen(dorifd, MAX);
    if (rc == -1)
        error("listen: dorifd");

    rc = listen(shellfd, MAX);
    if (rc == -1)
        error("listen: shellfd");

    rc = listen(tkfd, MAX);
    if (rc == -1)
        error("listen: tkfd");

    FD_ZERO(&master);
    FD_SET(tkfd, &master);
    FD_SET(dorifd, &master);
    FD_SET(shellfd, &master);
    FD_SET(0, &master);

    maxfd = tkfd > dorifd ? tkfd : dorifd;
    maxfd = shellfd > maxfd ?  shellfd : maxfd;

    for (;;)
    {
        readfds = master;

        rc = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (rc == -1)
            error("select");

        if(FD_ISSET(0, &readfds))
        {
            rc = read(0, buf, sizeof(buf));
            buf[rc] = '\0';

            int i;
            for(i = 0; i < nclients; i++)
            {
                if(clients[i].type == DORI)
                {
                    write(clients[i].fd, buf, strlen(buf));
                    break;
                }
            }
        }
        // for server connections
        for (fd = 1; fd <= maxfd; fd++)
        {
            if (!FD_ISSET(fd, &readfds))
            {
                continue;
            }

            if (fd == tkfd || fd == dorifd || fd == shellfd)
            {
                len = sizeof(clientaddr);
                newfd = accept(fd, (struct sockaddr *)&clientaddr, &len);
                clients[nclients].fd = newfd;

                if (newfd > maxfd)
                    maxfd = newfd;

                if(fd == tkfd)
                {
                    printf("new tk\n");
                    rc = read(newfd, buf, sizeof(buf));
                    buf[rc] = '\0';
                    int next_tk_rowid = atoi(buf);
                    clients[nclients].type = TK;
                    char query[256];
                    sprintf(query, "SELECT rowid, * FROM RECORDS where rowid >= %d", next_tk_rowid);
                    sqlite3_exec(db, query, sqlite_cb, &newfd, NULL);

                    if(send(newfd, "-1", 3, MSG_NOSIGNAL) < 1 ||
                       send(newfd, "", 1, MSG_NOSIGNAL) < 1   ||
                       send(newfd, "", 1, MSG_NOSIGNAL) < 1   ||
                       send(newfd, "", 1, MSG_NOSIGNAL) < 1   ||
                       send(newfd, "", 1, MSG_NOSIGNAL) < 1   ||
                       send(newfd, "", 1, MSG_NOSIGNAL) < 1   ||
                       send(newfd, "", 1, MSG_NOSIGNAL) < 1)
                    {
                        printf("err 3: client disconnected during write\n");
                        remove_client(fd);
                    }
                }
                else if(fd == shellfd)
                {
                    clients[nclients].type = SHELL;
                    active_shell_client = &clients[nclients];
                    printf("Shell connected\n");
                }
                else if(fd == dorifd)
                {
                    printf("DORI connection established\n");
                    clients[nclients].type = DORI;
                    active_dori_client = &clients[nclients];
                }

                nclients++;
                FD_SET(newfd, &master);

            } else {
                client *c = find_client(fd);
                rc = read(fd, buf, sizeof(buf));
                if (rc == 0 || (rc < 0 && errno == ECONNRESET))
                {
                    if(c->type == DORI)
                    {
                        // if DORI disconnects, then update shell's state
                        if(cur_shell_state > CONNECTED)
                        {
                            cur_shell_state = CONNECTED;
                        }
                        active_dori_client = NULL;
                        printf("DORI disconnected\n");
                    }
                    else if(c->type == SHELL)
                    {
                        if(active_shell_client && active_shell_client == c)
                        {
                            active_shell_client = NULL;
                            cur_shell_state = DISCONNECTED;
                            printf("Active ");
                        }
                        printf("shell disconnected\n");
                    }
                    remove_client(fd);

                } else if (rc < 0)
                {
                    error("read");
                } else {

                    if(c == NULL)
                    {
                        printf("couldn't find client!\n");
                        continue;
                    }
                    if(c->type == DORI)
                    {
                        process_dori_bytes(buf, rc);
                    }
                    else {
                        if(buf[rc-1] == '\n')
                            buf[rc-1] = '\0';
                        else
                            buf[rc] = '\0';

                        if(c->type == SHELL)
                        {
                            if(process_shell_msg(c, buf) < 0)
                            {
                                printf("Error processing shell command\n");
                            }
                        } else if(c->type == TK)
                        {
                            printf("tk wrote: %s\n", buf);
                        }
                    }
                }
            }
        }
    }
}
