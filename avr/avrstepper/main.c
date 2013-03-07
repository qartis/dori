#include <stdio.h>

#include "uart.h"
#include "motor.h"

int main(void) {
    uart_init(BAUD(38400));
    motor_init();

    char ch;
    for (;;) {
        ch = getchar();
        if (ch == 'a'){
            motor_ccw();
        } else if (ch == 's'){
            motor_cw();
        } else if (ch == 'x'){
            motor_stop();
        } else {
            motor_cw();
        }
    }
}
