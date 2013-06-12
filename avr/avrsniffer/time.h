extern volatile uint32_t now;
extern volatile uint32_t periodic_prev;
extern volatile uint16_t periodic_interval;

void time_init(void);
void time_set(uint32_t new_time);

void periodic_callback(void);
