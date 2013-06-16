#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <inttypes.h>
#include <poll.h>
#include <errno.h>

ssize_t saferead(int fd, char *buf, ssize_t count){
    ssize_t r;
    while (count > 0){
        r = read(fd, buf, count);
        if (r == 0) exit(0);
        buf += r;
        count -= r;
    }
    return 0;
}

ssize_t safewrite(int fd, const uint8_t *buf, ssize_t count){
    while (count>0){
        size_t wrote = write(fd, buf, count);
        buf += wrote;
        count -= wrote;
    }
    return count;
}

int main(int argc, char **argv){
    ssize_t r;
    int i;
    uint8_t buf[1024];
    int prev = -1;

    FILE *dev = fopen("/dev/ttyUSBblack","r+");
    assert(dev);

    int fd = fileno(dev);
    struct termios opt;
    tcgetattr(fd, &opt);
    cfsetspeed(&opt, B9600);
    tcsetattr(fd, TCSANOW, &opt);

    for(;;){
        double temp;
        if (fscanf(dev,"%lf",&temp) == 1){
            printf("temp: %0.1lf\n",temp);
        } else {
            usleep(50000);
        }
    }
}

