int8_t read_timeout(uint8_t *buf, size_t count, uint8_t sec);
uint8_t getline_timeout(char *s, uint8_t n, uint8_t sec);
void timer_start(void);
void delay_ms(uint16_t ms);
inline void timer_disable(void){
    TCCR1B = 0;
}


extern volatile uint8_t timeout_counter;
