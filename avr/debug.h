#define DEBUG_BUF_SIZE 64
#define DEBUG_TX_BUF_SIZE 128

void debug_init(void);
uint8_t debug_getchar(void);
void debug_putchar(char c);
void debug_flush(void);
