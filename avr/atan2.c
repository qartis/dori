#include <stdio.h>
#include <stdlib.h>

int16_t atan2_fp(int16_t y, int16_t x)
{
    const int16_t cordic[] = {450, 266, 140, 71, 36, 18, 9, 4, 2, 1};
    int16_t theta;
    int16_t i;

    if (x < 0) {
        theta = 1800;
        x = -x;
        y = -y;
    } else {
        theta = 0;
    }

    for (i = 0; i < 10; i++) {
        int16_t last_x;

        last_x = x;

        if (y < 0) { // sign=1
            x -= y >> i;
            y += last_x >> i;
            theta -= cordic[i];
        } else {
            x += y >> i;
            y -= last_x >> i;
            theta += cordic[i];
        }
    }

    return theta % 3600;
}

int main(int argc, char **argv)
{
    int16_t x = atoi(argv[1]);
    int16_t y = atoi(argv[2]);
    int16_t result = atan2_fp(y, x);
    result = result + 3600;
    result %= 3600;
    printf("%d\n", result);
}
