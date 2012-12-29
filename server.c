#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
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
sqlite3 *db_tmp;


struct live {
    const char *query;
    int fd;
} lives[MAX];
int nlives;

void error(const char *str)
{
    perror(str);
    exit(1);
}

int strprefix(const char *a, const char *b)
{
    return strncmp(a, b, strlen(b));
}

void remove_client(int fd)
{
    int i;

    close(fd);
    
    for (i = 0; i < nlives; i++) {
        if (lives[i].fd == fd) {
            printf("removed live '%s' for fd %d\n", lives[i].query, lives[i].fd);
            memcpy(&lives[i], &lives[i + 1],
                    (nlives - i - 1) * sizeof(lives[0]));
            nlives--;
            i--;
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
}

int sqlite_live_cb(void *arg, int nrows, char **cols, char **rows)
{
    int *fd = arg;
    int i;

    printfd(*fd, "live row: {");

    for (i = 0; i < nrows; i++) {
        if (i)
            printfd(*fd, ",");

        printfd(*fd, "%s", cols[i]);
    }

    printfd(*fd, "}\n");

    return 0;
}

int sqlite_cb(void *arg, int nrows, char **cols, char **rows)
{
    int *fd = arg;
    int i;

    printfd(*fd, "result row: {");

    for (i = 0; i < nrows; i++) {
        if (i)
            printfd(*fd, ",");

        printfd(*fd, "%s", cols[i]);
    }

    printfd(*fd, "}\n");

    return 0;
}

void runsql(char *query, int fd)
{
    int rc;
    int i;

    char *insert = strcasestr(query, "insert");
    char *select = strcasestr(query, "select");

    if ((insert && select) || (!insert && !select))
        perror("unknown query type");

    char *live = strstr(query, " live");
    if (live)
        *live = '\0';

    printfd(fd, "^");
    rc = sqlite3_exec(db, query, sqlite_cb, &fd, NULL);
    if (rc != SQLITE_OK) {
        perror(sqlite3_errmsg(db));
        printfd(fd, "%c%s", '!', sqlite3_errmsg(db));
    }

    if (select && live) {
        printf("adding live query %s %d\n", query, fd);
        lives[nlives].query = strdup(query);
        lives[nlives].fd = fd;
        nlives++;
    } else if (insert) {
        rc = sqlite3_exec(db_tmp, query, NULL, NULL, NULL);
        if (rc != SQLITE_OK)
            error(sqlite3_errmsg(db));

        for (i = 0; i < nlives; i++) {
            rc = sqlite3_exec(db_tmp, lives[i].query, sqlite_live_cb, &lives[i].fd, NULL);
            if (rc != SQLITE_OK)
                error(sqlite3_errmsg(db));
        }

        sqlite3_exec(db_tmp, "delete from records", NULL, NULL, NULL);
    }
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
    sqlite3_open("db_tmp", &db_tmp);

    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpfd == -1)
        error("tcp socket");

    optval = 1;
    setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

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
    maxfd = tcpfd;

    for (;;) {
        readfds = master;

        rc = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (rc == -1)
            error("select");

        for (fd = 0; fd <= maxfd; fd++) {
            if (!FD_ISSET(fd, &readfds)) {
                continue;
            }

            if (fd == tcpfd) {
                len = sizeof(clientaddr);
                newfd = accept(tcpfd, (struct sockaddr *)&clientaddr, &len);

                if (newfd > maxfd)
                    maxfd = newfd;

                FD_SET(newfd, &master);
            } else {
                rc = read(fd, buf, sizeof(buf));
                if (rc < 0)
                    error("read");

                buf[rc] = '\0';
                if (buf[rc-1] == '\n')
                    buf[rc-1] = '\0';

                if (rc == 0) {
                    FD_CLR(fd, &master);
                    remove_client(fd);
                } else {
                    runsql(buf, fd);
                }
            }
        }
    }
}
