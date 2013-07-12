void sram_hw_init(void);
uint8_t sram_init(void);
void sram_write(uint16_t addr, uint8_t val);
void sram_read_begin(uint16_t addr);
uint8_t sram_read_byte(void);
void sram_read_end(void);
