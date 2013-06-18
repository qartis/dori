#define MOTOR_DDR DDRC
#define MOTOR_PORT PORTC
#define MOTOR_BIT(n) (1<<(PORTC0 + n))

void motor_init(void);
void motor_cw(void);
void motor_ccw(void);
void motor_stop(void);
void motor_spin(void);
