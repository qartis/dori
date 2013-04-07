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

#define MAX 1024
#define DORIPORT 53
#define TKPORT 1337
#define SHELLPORT 1338
#define BUFLEN 4096

sqlite3 *db;

typedef enum {
    DORI,
    TK,
    SHELL,
} client_type;

typedef struct {
    int fd;
    client_type type;
} client;

char timestamp[128];
static client clients[128];
static int nclients;
static int dorifd;
static int tkfd;
static int shellfd;

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
    return strncmp(a, b, strlen(b));
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
}

ssize_t safe_write(int fd, const char *buf, size_t count)
{
    ssize_t rc;
    size_t wroteb;

    wroteb = 0;
    while (wroteb < count) {
        rc = write(fd, buf + wroteb, count - wroteb);
        if (rc < 1)
            error("write");

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
        write(*fd, "", 1);
    }

    return 0;
}

int main()
{
    int newfd, maxfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    socklen_t len;
    fd_set master;
    fd_set readfds;
    int optval, rc, fd;
    char buf[BUFLEN];

    sqlite3_open("db", &db);

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
            char buf[256];
            read(0, buf, sizeof(buf));
            char *types[] = {
                "gps", "laser", "accel", "temp", "arm", "plate", "orientation"
            };
            int type = rand() % (sizeof(types) / sizeof(*types));
            int a, b, c;
            a = rand() % 360;
            b = rand() % 100;
            c = rand() % 100;

            sprintf(buf, "INSERT INTO records (type, a, b, c) VALUES ('%s', %d, %d, %d)", types[type], a, b, c);
            sqlite3_exec(db, buf, NULL, NULL, NULL);
            int64_t rowid = sqlite3_last_insert_rowid(db);
            printf("new row: rowid: %ld, %s, %d, %d, %d\n", rowid, types[type], a, b, c);
            sprintf(buf, "SELECT rowid, * FROM records WHERE rowid = '%ld';", rowid);

            int i;
            for (i = 0; i < nclients; i++) {
                rc = sqlite3_exec(db, buf, sqlite_cb,
                                  &clients[i], NULL);

                if (rc != SQLITE_OK)
                    error(sqlite3_errmsg(db));
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
                FD_SET(newfd, &master);

                if (newfd > maxfd)
                    maxfd = newfd;

                if(fd == tkfd) {
                    printf("new tk\n");
                    clients[nclients].type = TK;
                    sqlite3_exec(db, "SELECT rowid, * FROM RECORDS", sqlite_cb, &newfd, NULL);

                    write(newfd, "-1", 3);
                    write(newfd, "", 1);
                    write(newfd, "", 1);
                    write(newfd, "", 1);
                    write(newfd, "", 1);
                    write(newfd, "", 1);
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
            } else {
                rc = read(fd, buf, sizeof(buf));
                if (rc == 0 || (rc < 0 && errno == ECONNRESET)) {
                    FD_CLR(fd, &master);
                    remove_client(fd);
                    printf("client died\n");
                } else if (rc < 0) {
                    error("read");
                } else {
                    if(buf[rc-1] == '\n')
                        buf[rc-1] = '\0';
                    else
                        buf[rc] = '\0';

                    client *c = find_client(fd);

                    if(c == NULL) {
                        printf("couldn't find client!\n");
                        continue;
                    }
                    if(c->type == SHELL) {
                        printf("shell wrote: %s\n", buf);
                    } else if(c->type == DORI) {
                        printf("dori wrote: %s\n", buf);
                    } else if(c->type == TK) {
                        printf("tk wrote: %s\n", buf);
                    }
                }
            }
        }
    }
}
