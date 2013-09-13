#define MOTOR_DDR DDRD
#define MOTOR_PORT PORTD
#define MOTOR_BIT(n) (1<<(PORTD4 + n))

void motor_init(void);
void motor_cw(void);
void motor_ccw(void);
void motor_stop(void);
void motor_spin(void);
