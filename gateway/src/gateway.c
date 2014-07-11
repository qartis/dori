#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include "can.h"
#include "can_names.h"

#define MAX 1024
#define DORIPORT 53
#define SHELLPORT 1338
#define BUFLEN 4096

#define CAN_TYPE_IDX 0
#define CAN_ID_IDX 1
#define CAN_SENSOR_IDX 2
#define CAN_LEN_IDX 4
#define CAN_HEADER_LEN 5

#define LOGFILE_SIZE 100

FILE *incoming_file;

sqlite3 *db;
fd_set master;

typedef enum {
    DORI = 1,
    SHELL,
} client_type;

typedef struct {
    int fd;
    client_type type;
    unsigned char buf[MAX];
    int buf_len;
} client;

static client clients[MAX];
static int nclients;
static int dorifd;
static int shellfd;
static int siteid;

const char *ptime(void)
{
    time_t t;
    struct tm *tm;
    static char buf[64];

    time(&t);
    tm = localtime(&t);
    strftime(buf, sizeof(buf), "%c", tm);

    return buf;
}

#define log(args...) do { \
        printf("[%s] ", ptime()); \
        printf(args); \
    } while (0);

void error(const char *str)
{
    perror(str);
    exit(1);
}

void dberror(sqlite3 * database)
{
    log("db error: %s\n", sqlite3_errmsg(database));
    exit(1);
}

client *find_client_type(client_type type)
{
    int i;

    for (i = 0; i < nclients; i++) {
        if (clients[i].type == type) {
            return &clients[i];
        }
    }

    return NULL;
}

client *find_client(int fd)
{
    int i;

    for (i = 0; i < nclients; i++) {
        if (clients[i].fd == fd) {
            return &clients[i];
        }
    }

    return NULL;
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

size_t write_to_client(client_type target_client, const char* buf, int buflen)
{
    int num_clients_written;
    int i;
    int rc;

    num_clients_written = 0;
    for (i = 0; i < nclients; i++) {

        if (clients[i].type == target_client) {

            num_clients_written++;
            rc = write(clients[i].fd, buf, buflen);
            if(rc <= 0) {
                log("Error writing to client type %d", target_client);
                remove_client(clients[i].fd);
                return -1;
            }
        }
    }

    return num_clients_written;
}


ssize_t safe_write(int fd, const char *buf, size_t count)
{
    ssize_t rc;
    size_t wroteb;

    wroteb = 0;
    while (wroteb < count) {
        rc = write(fd, buf + wroteb, count - wroteb);
        if (rc < 1) {
            remove_client(fd);
            log("err 1: client disconnected during write\n");
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
        if (write(*fd, "", 1) < 1) {
            log("err 2: client disconnected during write\n");
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
    if (ncols == 1 && cols[0] != '\0') {
        siteid = atoi(cols[0]);
    }
    return 0;
}

void exec_query(char *query)
{
    int rc = sqlite3_exec(db, query, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        log("sqlite error: %s\n", sqlite3_errmsg(db));
    }
}

void db_log_can(uint8_t type, uint8_t id, uint16_t sensor, uint8_t len,
    const uint8_t *data)
{
        sqlite3_stmt *stmt;
        int rc;
        int i;
        char data_str[8 * 2 + 1];

        data_str[0] = '\0';

        rc = sqlite3_prepare_v2(db, "insert into can (type, id, sensor, data) values (?,?,?,?)", -1, &stmt, NULL);
        if (rc != SQLITE_OK) dberror(db);

        for (i = 0; i < len; i++) {
            sprintf(data_str + (i * 2), "%02x", data[i]);
        }

        rc = sqlite3_bind_text(stmt, 1, type_names[type], strlen(type_names[type]), SQLITE_STATIC);
        if (rc != SQLITE_OK) dberror(db);

        rc = sqlite3_bind_text(stmt, 2, id_names[id], strlen(id_names[id]), SQLITE_STATIC);
        if (rc != SQLITE_OK) dberror(db);

        rc = sqlite3_bind_text(stmt, 3, sensor_names[sensor], strlen(sensor_names[sensor]), SQLITE_STATIC);
        if (rc != SQLITE_OK) dberror(db);

        rc = sqlite3_bind_text(stmt, 4, data_str, strlen(data_str), SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) dberror(db);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) dberror(db);
}

void process_dori_bytes(client* c)
{
    while (c->buf_len > 0) {
        uint8_t type = c->buf[CAN_TYPE_IDX];

        if (type == TYPE_nop) {
            log("skipping nop\n");
            c->buf_len -= 1;
            memmove(c->buf, c->buf + 1, c->buf_len);
            continue;
        }

        if (c->buf_len < CAN_HEADER_LEN) {
            break;
        }

        // Extract the payload size from the CAN header
        uint8_t data_len = c->buf[CAN_LEN_IDX];

        if (data_len > 8) {
            log("data_len > 8: %d\n", data_len);
            data_len = 8;
        }

        // Break out if we don't have a full packet yet
        if (c->buf_len < CAN_HEADER_LEN + data_len) {
            return;
        }

        uint8_t id = c->buf[CAN_ID_IDX];
        uint16_t sensor = (c->buf[CAN_SENSOR_IDX] << 8) |
                           c->buf[CAN_SENSOR_IDX + 1];

        // Sanity checks here to prevent future crashes
        if (type >= TYPE_invalid) {
            log("Invalid type: %02x\n", type);

            c->buf_len -= (CAN_HEADER_LEN + data_len);

            break;
        }

        if (id >= ID_invalid) {
            log("Invalid id: %02x\n", id);

            c->buf_len -= (CAN_HEADER_LEN + data_len);

            memmove(c->buf, c->buf + CAN_HEADER_LEN + data_len,
                    (c->buf_len) * sizeof(unsigned char));
            break;
        }

        if(sensor >= SENSOR_invalid) {
            log("Invalid sensor: %02x\n", sensor);
            c->buf_len -= (CAN_HEADER_LEN + data_len);

            memmove(c->buf, c->buf + CAN_HEADER_LEN + data_len,
                    (c->buf_len) * sizeof(unsigned char));
            break;
        }

        unsigned char *data = c->buf + CAN_HEADER_LEN;

        log("DORI sent Frame: %s [%02x] %s [%02x] %s [%02x] %d [",
               type_names[type],
               type,
               id_names[id],
               id,
               sensor_names[sensor],
               sensor,
               data_len);

        int i;

        for (i = 0; i < data_len; i++) {
            printf(" %02x ", data[i]);
        }

        printf("]\n");

        switch (type) {
        case TYPE_ircam_header:
            {
                struct timeval timestamp;
                gettimeofday(&timestamp,NULL);
                char file_path[128];
                sprintf(file_path, "ircam/%ld.jpg", timestamp.tv_sec);
                incoming_file = fopen(file_path, "w");
                break;
            }
        case TYPE_laser_sweep_header:
            {
                uint16_t start_angle;
                uint16_t end_angle;
                uint16_t stepsize;
                struct timeval timestamp;
                char file_path[128];

                start_angle = data[0] << 8 | data[1];
                end_angle   = data[2] << 8 | data[3];
                stepsize    = data[4] << 8 | data[5];

                gettimeofday(&timestamp,NULL);
                sprintf(file_path, "laser/%ld - %u %u %u.dat", timestamp.tv_sec,
                        start_angle, end_angle, stepsize);
                incoming_file = fopen(file_path, "w");
                break;
            }
        case TYPE_xfer_chunk:
            if(incoming_file != NULL) {
                fwrite(c->buf + CAN_HEADER_LEN, data_len, 1, incoming_file);
                fflush(incoming_file);
                fflush(incoming_file);
                fsync(fileno(incoming_file));
                fsync(fileno(incoming_file));
            }
            break;
        }


        db_log_can(type, id, sensor, data_len, c->buf + CAN_HEADER_LEN);

        int rc = 0;

        for (i = 0; i < nclients; i++) {
            uint8_t sensor_buf[2];
            sensor_buf[0] = sensor >> 8;
            sensor_buf[1] = sensor & 0xff;

            if (clients[i].type == SHELL) {
                rc = write(clients[i].fd, &type, sizeof(type));
                if(rc < 0) break;

                rc = write(clients[i].fd, &id, sizeof(id));
                if(rc < 0) break;

                rc = write(clients[i].fd, sensor_buf, sizeof(sensor_buf));
                if(rc < 0) break;

                rc = write(clients[i].fd, &data_len, sizeof(data_len));
                if(rc < 0) break;

                rc = write(clients[i].fd, c->buf + CAN_HEADER_LEN, data_len);
                if(rc < 0) break;
            }
        }

        if(rc < 0) {
            perror("Failed to write DORI bytes to shell");
            remove_client(clients[i].fd);
        }

        c->buf_len -= (CAN_HEADER_LEN + data_len);

        memmove(c->buf, c->buf + CAN_HEADER_LEN + data_len,
                (c->buf_len) * sizeof(unsigned char));
    }
}

void process_shell_bytes(client* c)
{
    // if the command that shell sends is unrecognized
    // we'll parse it as a CAN command
    if (c->buf_len < CAN_HEADER_LEN) {
        return;
    }
    // Extract the payload size from the CAN header
    uint8_t data_len = c->buf[CAN_LEN_IDX];

    // Break out if we don't have a full packet yet
    if (c->buf_len < CAN_HEADER_LEN + data_len) {
        return;
    }

    uint8_t type = c->buf[CAN_TYPE_IDX];
    uint8_t id = c->buf[CAN_ID_IDX];
    uint16_t sensor = (c->buf[CAN_SENSOR_IDX] << 8) | c->buf[CAN_SENSOR_IDX + 1];

    // Sanity checks here to prevent future crashes
    if (type >= TYPE_invalid) {
        log("Invalid type: %02x\n", type);

        c->buf_len -= (CAN_HEADER_LEN + data_len);

        return;
    }

    if (id >= ID_invalid) {
        log("Invalid id: %02x\n", id);

        c->buf_len -= (CAN_HEADER_LEN + data_len);

        memmove(c->buf, c->buf + CAN_HEADER_LEN + data_len,
                (c->buf_len) * sizeof(unsigned char));
        return;
    }

    if(sensor >= SENSOR_invalid) {
        log("Invalid sensor: %02x\n", sensor);
        c->buf_len -= (CAN_HEADER_LEN + data_len);

        memmove(c->buf, c->buf + CAN_HEADER_LEN + data_len,
                (c->buf_len) * sizeof(unsigned char));
        return;
    }


    log("shell sent frame: %s [%02x] %s [%02x] %s [%04x] %d [",
           type_names[type],
           type,
           id_names[id],
           id,
           sensor_names[sensor],
           sensor,
           data_len);

    int i;
    for (i = 0; i < data_len; i++) {
        printf(" %02x ", c->buf[CAN_HEADER_LEN + i]);
    }

    printf("]\n");

    int rc;
    rc = write_to_client(DORI, (const char*)c->buf, c->buf_len);

    if(rc == 0) {
        log("No DORI available\n");
    } else if(rc < 0) {
        log("Error while writing to DORI\n");
    } else {
        log("Sending frame to %d clients: %s [%02x] %s [%02x] %s [%04x] %d [",
               rc,
               type_names[type],
               type,
               id_names[id],
               id,
               sensor_names[sensor],
               sensor,
               data_len);
        int j;
        for (j = 0; j < data_len; j++) {
            printf(" %02x ", c->buf[CAN_HEADER_LEN + j]);
        }
        printf("\n");
    }

    c->buf_len -= (CAN_HEADER_LEN + data_len);

    memmove(c->buf, &c->buf[CAN_HEADER_LEN + data_len],
            (c->buf_len) * sizeof(char));
}

int main()
{
    int newfd, maxfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    socklen_t len;
    fd_set readfds;
    int optval, rc, fd;

    signal(SIGPIPE, SIG_IGN);

    sqlite3_open("data/db", &db);

    if (db) {
        sqlite3_exec(db, "select max(site) from records;", site_cb, NULL, NULL);
    }

    // Setup some expected directories
    system("mkdir -p files");
    system("mkdir -p ircam");
    system("mkdir -p laser");

    dorifd = socket(AF_INET, SOCK_STREAM, 0);
    if (dorifd == -1)
        error("dori socket");

    shellfd = socket(AF_INET, SOCK_STREAM, 0);
    if (shellfd == -1)
        error("shell socket");

    optval = 1;
    setsockopt(dorifd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    setsockopt(shellfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

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

    rc = listen(dorifd, MAX);
    if (rc == -1)
        error("listen: dorifd");

    rc = listen(shellfd, MAX);
    if (rc == -1)
        error("listen: shellfd");

    FD_ZERO(&master);
    FD_SET(dorifd, &master);
    FD_SET(shellfd, &master);
    //FD_SET(0, &master);

    maxfd = shellfd;
    if (dorifd > maxfd) {
        maxfd = dorifd;
    }

    for (;;) {
        readfds = master;

        struct timeval tv;
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        rc = select(maxfd + 1, &readfds, NULL, NULL, &tv);
        if (rc == -1) {
            error("select");
        } else if (rc == 0) {
            char nop = TYPE_nop;
            write_to_client(DORI, &nop, sizeof(nop));
            log("sending nop\n");

            /*
            client *dori = find_client_type(DORI);
            if (dori != NULL) {
                FD_CLR(dori->fd, &master);
                remove_client(dori->fd);
                log("DORI disconnected\n");
            }
            */
            continue;
        }

        for (fd = 1; fd <= maxfd; fd++) {
            if (!FD_ISSET(fd, &readfds)) {
                continue;
            }

            if (fd == dorifd || fd == shellfd) {
                len = sizeof(clientaddr);
                newfd = accept(fd, (struct sockaddr *)&clientaddr, &len);

                if (nclients == MAX) {
                    log("Max clients reached\n");
                    exit(1);
                }

                clients[nclients].buf_len = 0;
                memset(clients[nclients].buf, '\0', sizeof(clients[nclients].buf));
                clients[nclients].fd = newfd;

                int flags = fcntl(clients[nclients].fd, F_GETFL, 0);
                fcntl(clients[nclients].fd, F_SETFL, flags | O_NONBLOCK);

                if (newfd > maxfd)
                    maxfd = newfd;

                if (fd == shellfd) {
                    clients[nclients].type = SHELL;
                    log("shell connected\n");
                } else if (fd == dorifd) {
                    log("DORI connected\n");
                    clients[nclients].type = DORI;
                }

                nclients++;
                FD_SET(newfd, &master);

            } else {
                client *c = find_client(fd);
                rc = read(fd, c->buf + c->buf_len, sizeof(c->buf) - c->buf_len);
                if (rc == 0 || rc < 0) {
                    if (c->type == DORI) {
                        log("DORI disconnected\n");
                    } else if (c->type == SHELL) {
                        log("shell disconnected\n");
                    } else {
                        log("unknown client disconnected\n");
                        exit(1);
					}
                    remove_client(fd);

                } else {
                    c->buf_len += rc;
                    log("Got %d bytes from fd %d: ", rc, fd);
                    int j = 0;
                    for (j = 0; j < rc; j++) {
                        printf("%02x(%c) ", (unsigned char)c->buf[j],
                                isprint(c->buf[j]) ? c->buf[j] : '?');
                    }
                    printf("\n");

                    if (c == NULL) {
                        log("couldn't find client!\n");
                        exit(1);
                    }
                    if (c->type == DORI) {
                        process_dori_bytes(c);
                    } else if (c->type == SHELL) {
                        process_shell_bytes(c);
                    } else {
                        log("unknown type: %d\n", c->type);
                    }
                }
            }
        }
    }
}
