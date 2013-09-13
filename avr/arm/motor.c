#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "motor.h"

uint8_t steps[] = {
    (MOTOR_BIT(3)               ),
    (               MOTOR_BIT(1)),
    (MOTOR_BIT(2)               ),
    (               MOTOR_BIT(0)),
};

#define NUM_STEPS (sizeof(steps)/sizeof(*steps))
#define DELAY 15

int step;

void motor_init(void)
{
    MOTOR_DDR = 0xff;
    step = 0;
    motor_stop();
}

void motor_ccw(void)
{
    step++;
    step %= NUM_STEPS;
    MOTOR_PORT = steps[step];
    _delay_ms(DELAY);
    motor_stop();
}

void motor_cw(void)
{
    if (step == 0) {
        step = NUM_STEPS - 1;
    } else {
        step--;
    }
    MOTOR_PORT = steps[step];
    printf("port %x\n", MOTOR_PORT);
    _delay_ms(DELAY);
    motor_stop();
}

void motor_stop(void)
{
    MOTOR_PORT = 0;
}
