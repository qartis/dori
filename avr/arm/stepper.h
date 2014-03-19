#define STEPPER_DDR DDRD
#define STEPPER_PORT PORTD
#define STEPPER_BIT(n) (1<<(PORTD4 + n))

#define STEPPER_SLP PORTC5

extern int32_t stepper_state;

void stepper_init(void);
void stepper_cw(void);
void stepper_ccw(void);
void stepper_sleep(void);
void stepper_wake(void);
uint16_t stepper_get_stepsize(void);
uint8_t stepper_set_stepsize(uint16_t stepsize);
int32_t stepper_get_state(void);
uint8_t stepper_set_state(int32_t new_state);
uint8_t stepper_set_angle(uint16_t goal);

