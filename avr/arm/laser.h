void laser_init(void);
void laser_off(void);
void laser_on(void);
void laser_on_safe(void);

// assumes laser is already on
uint16_t measure_once(void);

// reads up to 100 measurements
int8_t measure_rapid_fire(uint8_t target_count);

// assumes laser is already on
void measure(uint32_t target_count);
