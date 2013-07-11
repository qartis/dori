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
#include "candefs.h"
#include "command.h"
#include <signal.h>

#define MAX 1024
#define DORIPORT 53
#define TKPORT 1337
#define SHELLPORT 1338
#define BUFLEN 4096

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


typedef struct {
    unsigned char type;
    unsigned char id;
    unsigned char data[10];
} dori_msg;

char *msg_ids[] = { "any", "ping", "pong", "laser", "gps", "temp", "time", "log", "invalid" };

char timestamp[128];
static client clients[128];
static int nclients;
static int dorifd;
static int tkfd;
static int shellfd;
static int siteid;

typedef enum {
    DISCONNECTED,
    CONNECTED,
    FILE_TRANSFER
} shell_state;

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

    for(i = 0; i < nclients; i++) {
        if(clients[i].fd == fd) {
            return &clients[i];
        }
    }

    return c;
}

void remove_client(int fd)
{
    int i;

    close(fd);

    for (i = 0; i < nclients; i++) {
        if (clients[i].fd == fd) {
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
    while (wroteb < count) {
        rc = send(fd, buf + wroteb, count - wroteb, MSG_NOSIGNAL);
        if (rc < 1) {
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

    for (i = 0; i < ncols; i++) {
        printfd(*fd, "%s", cols[i]);
        if(send(*fd, "", 1, MSG_NOSIGNAL) < 1) {
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
    if(ncols == 1 && cols[0] != '\0') {
        siteid = atoi(cols[0]);
    }
    return 0;
}


void exec_query(char *query) {
    int rc = sqlite3_exec(db, query, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        printf("sqlite error: %s\n", sqlite3_errmsg(db));
    }
}


void process_dori_msg(dori_msg msg) {
    printf("DORI sent msg:\n");
    printf("type: %u\n", msg.type);
    printf("id: %u\n", msg.id);
    int i;
    printf("data: ");
    for(i = 9; i >= 0; i--) {
        printf("%u\n", msg.data[i]);
    }

    printf("\n");

    char buf[256];

    switch(msg.id) {
    case ID_LASER:
        {
            int a = (msg.data[0] & 0xFF) | ((msg.data[1] & 0xFF) << 8);
            int b = (msg.data[2] & 0xFF) | ((msg.data[3] & 0xFF) << 8);
            int c = 0;

            printf("laser angle: %d, dist: %d\n", a, b);
            sprintf(buf, "INSERT INTO records (type, a, b, c, site) VALUES ('%s', %d, %d, %d, %d)", msg_ids[msg.id], a, b, c, siteid);
            exec_query(buf);
        }
        break;
    case ID_GPS:
        break;
    case ID_TEMP:
        {
            int a = (msg.data[0] & 0xFF) | ((msg.data[1] & 0xFF) << 8);
            int b = 0;
            int c = 0;

            printf("temp: %d\n", a);
            sprintf(buf, "INSERT INTO records (type, a, b, c, site) VALUES ('%s', %d, %d, %d, %d)", msg_ids[msg.id], a, b, c, siteid);
            exec_query(buf);
        }
        break;
    case ID_TIME:
        break;
    case ID_LOGGER:
        break;
    case ID_DRIVE:
        siteid++;
        break;
    }
}

void process_file_transfer(void *data, char *argv[]) {
    int node_index = 1;
    int filename_index = 2;
    printf("called process file transfer\n");

    printf("Requesting file '%s' from node '%s'\n",  argv[filename_index], argv[node_index]);

    // TODO create message that DORI will understand

    // 0. make a shell wrapper that tells gateway that shell is alive
    // 1. send file transfer init message to DORI
    // 2. wait for DORI's response, which will have
    // the length of the file (TOTAL_BYTES)
    // 3. send CTS
    // 4. while NUM_BYTES < TOTAL_BYTES
    //    receive up to 8 byte chunks
    //    send them to shell
    // 5.

    /*

    */
}

int process_shell_msg(client *c, char *msg) {
    printf("msg: %s\n", msg);
    if(strprefix(msg, "GET")) {
        char buf[128];
        strcpy(buf, msg);

        int num_args = 0;

        char *command = strtok(buf, " ");

        char *node_id = strtok(NULL, " ");
        if(node_id != NULL) num_args++;

        char *filename = strtok(NULL, " ");
        if(filename != NULL) num_args++;

        char *argv[] = {command, node_id, filename};

        int i;

        for(i = 0; i < total_shell_commands; i++) {
            if(commands[i].args == num_args)
            {
                commands[i].func(&commands[i], argv);
                return 1;
            }
            else {
                printf("Invalid number of arguments for command %s. Expected %d, got %d.\n", command, commands[i].args, num_args);
            }
        }

    }
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

    if(db) {
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

    for (;;) {
        readfds = master;

        rc = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (rc == -1)
            error("select");

        if(FD_ISSET(0, &readfds)) {
            rc = read(0, buf, sizeof(buf));
            buf[rc] = '\0';

            int i;
            for(i = 0; i < nclients; i++) {
                if(clients[i].type == DORI) {
                    write(clients[i].fd, buf, strlen(buf));
                    break;
                }
            }
        }
        // for server connections
        for (fd = 1; fd <= maxfd; fd++) {
            if (!FD_ISSET(fd, &readfds)) {
                continue;
            }

            if (fd == tkfd || fd == dorifd || fd == shellfd) {
                len = sizeof(clientaddr);
                newfd = accept(fd, (struct sockaddr *)&clientaddr, &len);
                clients[nclients].fd = newfd;

                if (newfd > maxfd)
                    maxfd = newfd;

                if(fd == tkfd) {
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
                       send(newfd, "", 1, MSG_NOSIGNAL) < 1) {
                        printf("err 3: client disconnected during write\n");
                        remove_client(fd);
                    }
                }
                else if(fd == shellfd) {
                    clients[nclients].type = SHELL;
                    printf("Shell connected\n");
                }
                else if(fd == dorifd) {
                    clients[nclients].type = DORI;
                    printf("DORI connection established\n");
                }

                nclients++;
                FD_SET(newfd, &master);

            } else {
                rc = read(fd, buf, sizeof(buf));
                if (rc == 0 || (rc < 0 && errno == ECONNRESET)) {
                    remove_client(fd);
                    printf("client died\n");
                } else if (rc < 0) {
                    error("read");
                } else {
                    client *c = find_client(fd);

                    if(c == NULL) {
                        printf("couldn't find client!\n");
                        continue;
                    }
                    if(c->type == DORI) {
                        dori_msg msg = { 0 };
                        memcpy((void*)&msg, buf, rc);
                        process_dori_msg(msg);
                    }
                    else {
                        if(buf[rc-1] == '\n')
                            buf[rc-1] = '\0';
                        else
                            buf[rc] = '\0';

                        if(c->type == SHELL) {
                            process_shell_msg(c, buf);
                        } else if(c->type == TK) {
                            printf("tk wrote: %s\n", buf);
                        }
                    }
                }
            }
        }
    }
}
