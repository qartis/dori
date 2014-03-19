extern volatile uint16_t dist_mm;

typedef uint8_t (*laser_callback_t)(uint16_t dist);

void laser_init(void);
void laser_off(void);
void laser_on(void);
void laser_on_safe(void);
void laser_begin_rapidfire(void);
uint8_t laser_sweep(laser_callback_t cb);

// assumes laser is already on
uint16_t measure_once(void);

uint8_t measure_rapid_fire(laser_callback_t cb);
