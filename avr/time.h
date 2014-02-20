extern volatile uint32_t now;
extern volatile uint32_t periodic_prev;
extern volatile uint16_t periodic_interval;

#ifdef TIMER_TOPHALF
void timer_tophalf(void);
#endif

void time_init(void);
void time_set(uint32_t new_time);

uint8_t periodic_irq(void);
