#include <avr/io.h>
#include <util/delay.h>

#include "motor.h"

uint8_t steps[] = {(MOTOR_BIT(3) | MOTOR_BIT(1)),
                   (               MOTOR_BIT(1)),
                   (MOTOR_BIT(2) | MOTOR_BIT(1)),
                   (MOTOR_BIT(2)               ),
                   (MOTOR_BIT(2) | MOTOR_BIT(0)),
                   (               MOTOR_BIT(0)),
                   (MOTOR_BIT(3) | MOTOR_BIT(0)),
                   (MOTOR_BIT(3)               )};

#define NUM_STEPS (sizeof(steps)/sizeof(*steps))
#define DELAY 5

int step;

void motor_init(void)
{
    MOTOR_DDR |= (MOTOR_BIT(0) | MOTOR_BIT(1) | MOTOR_BIT(2) | MOTOR_BIT(3));
    step = 0;
    MOTOR_PORT = steps[step];
    _delay_ms(DELAY);
    motor_stop();
}

void motor_ccw(){
    step++;
    step %= NUM_STEPS;
    MOTOR_PORT = steps[step];
    _delay_ms(DELAY);
    motor_stop();
    /*
    uint8_t i;
    for(i=0;i<NUM_STEPS;i++){
        MOTOR_PORT = steps[i];
        _delay_ms(DELAY);
    }
    */
}

void motor_cw(void)
{
    if (step == 0) {
        step = NUM_STEPS - 1;
    } else {
        step--;
    }
    MOTOR_PORT = steps[step];
    _delay_ms(DELAY);
    motor_stop();
    /*
    int8_t i;
    for(i=NUM_STEPS - 1;i>=0;i--){
        MOTOR_PORT = steps[i];
        _delay_ms(DELAY);
    }
    */
}

void motor_stop(){
    MOTOR_PORT &= ~(MOTOR_BIT(0) | MOTOR_BIT(1) | MOTOR_BIT(2) | MOTOR_BIT(3));
}
