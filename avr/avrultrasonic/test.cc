#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

int readpackets(char *buf, int len)
{
    char *nl;
    int i;

    i = 0;

    for (;;) {
        nl = memchr(buf + i, '\n', len - i);
        if (nl == NULL) {
            break;
        }

        *nl = '\0';
        fprintf(stderr, "packet: '%s'\n", buf + i);
        fflush(stderr);

        i = nl - buf + 1;
    }

    memcpy(buf, buf + i, len - i);

    return len - i;
}


int main(int argc, char **argv)
{
    int fd;
    ssize_t rc;
    char buf[10];
    int len;

    setbuf(stderr, NULL);

    fd = open("/dev/ttyUSB1", O_RDONLY | O_NOCTTY);
    perror("open");

    len = 0;

    for (;;) {
        rc = read(fd, buf + len, sizeof(buf) - len);
        if (rc < 1) {
            perror("read");
            break;
        }
        len += rc;
        len = readpackets(buf, len);
    }
}

