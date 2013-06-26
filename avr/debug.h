void debug_init(void);
void debug(const char *fmt, ...);
int debug_getchar(void);
void debug_reset(void);
void debug_putchar(uint8_t c);

extern volatile uint8_t debug_buf[64];
extern volatile uint8_t debug_has_line;
