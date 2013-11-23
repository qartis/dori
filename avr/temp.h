extern uint8_t num_sensors;

void temp_init(void);
uint8_t temp_begin(void);
void temp_wait(void);
void temp_read(uint8_t channel, int16_t *temp);

