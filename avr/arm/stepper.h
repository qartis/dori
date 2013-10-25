#define STEPPER_DDR DDRD
#define STEPPER_PORT PORTD
#define STEPPER_BIT(n) (1<<(PORTD4 + n))

void stepper_init(void);
void stepper_cw(void);
void stepper_ccw(void);
void stepper_stop(void);
void stepper_spin(void);
void set_stepper_angle(int16_t goal);
uint16_t get_stepper_angle(void);
