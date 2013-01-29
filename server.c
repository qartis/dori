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

#define MAX 1024
#define PORT 1337
#define BUFLEN 4096

sqlite3 *db;

static int fds[128];
static int nfds;

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

void remove_client(int fd)
{
    int i;

    close(fd);

    for (i = 0; i < nfds; i++) {
        if (fds[i] == fd) {
            memmove(&fds[i], &fds[i + 1],
                    (nfds - i - 1) * sizeof(fds[0]));
            nfds--;
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
    int tcpfd, newfd, maxfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    socklen_t len;
    fd_set master;
    fd_set readfds;
    int optval, rc, fd;
    char buf[BUFLEN];

    sqlite3_open("db", &db);

    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpfd == -1)
        error("tcp socket");

    optval = 1;
    setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR,
        &optval, sizeof(optval));

    memset(&servaddr, '\0', sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    rc = bind(tcpfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (rc == -1)
        error("bind: tcp");

    rc = listen(tcpfd, MAX);
    if (rc == -1)
        error("listen: tcp");

    FD_ZERO(&master);
    FD_SET(tcpfd, &master);
    FD_SET(0, &master);
    maxfd = tcpfd;

    for (;;) {
        readfds = master;

        rc = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (rc == -1)
            error("select");

        if(FD_ISSET(0, &readfds)) {
            char buf[256];
            read(0, buf, sizeof(buf));
            char *types[] = {
                "gps", "laser", "accel", "temp"
            };
            int type = rand() % 4;
            int a, b, c;
            a = rand() % 100;
            b = rand() % 100;
            c = rand() % 100;

            sprintf(buf, "INSERT INTO records (type, a, b, c) VALUES ('%s', '%d', '%d', '%d')", types[type], a, b, c);
            sqlite3_exec(db, buf, NULL, NULL, NULL);
            int64_t rowid = sqlite3_last_insert_rowid(db);
            printf("new row: rowid: %ld, %s, %d, %d, %d\n", rowid, types[type], a, b, c);
            sprintf(buf, "SELECT rowid, * FROM records WHERE rowid = '%ld';", rowid);


            int i;
            for (i = 0; i < nfds; i++) {
                rc = sqlite3_exec(db, buf, sqlite_cb,
                                  &fds[i], NULL);

                if (rc != SQLITE_OK)
                    error(sqlite3_errmsg(db));
            }
        }
        // for server connections
        for (fd = 1; fd <= maxfd; fd++) {
            if (!FD_ISSET(fd, &readfds)) {
                continue;
            }

            if (fd == tcpfd) {
                len = sizeof(clientaddr);
                newfd = accept(tcpfd, (struct sockaddr *)&clientaddr, &len);
                fds[nfds++] = newfd;

                sqlite3_exec(db, "SELECT rowid, * FROM RECORDS", sqlite_cb, &newfd, NULL);

                write(newfd, "-1", 3);
                write(newfd, "", 1);
                write(newfd, "", 1);
                write(newfd, "", 1);
                write(newfd, "", 1);
                write(newfd, "", 1);

                if (newfd > maxfd)
                    maxfd = newfd;

                FD_SET(newfd, &master);
            } else {
                rc = read(fd, buf, sizeof(buf));
                if (rc == 0 || (rc < 0 && errno == ECONNRESET)) {
                    FD_CLR(fd, &master);
                    remove_client(fd);
                    printf("client died\n");
                } else if (rc < 0)
                    error("read");

                printf("client sent data\n");
            }
        }
    }
}
