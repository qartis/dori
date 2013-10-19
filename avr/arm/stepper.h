#define STEPPER_DDR DDRD
#define STEPPER_PORT PORTD
#define STEPPER_BIT(n) (1<<(PORTD4 + n))

void stepper_init(void);
void stepper_cw(void);
void stepper_ccw(void);
void stepper_stop(void);
void stepper_spin(void);
