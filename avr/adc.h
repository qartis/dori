int16_t map(int16_t x, int16_t in_min, int16_t in_max, 
        int16_t out_min, int16_t out_max);
void adc_init(void);
uint16_t adc_read(uint8_t channel);
