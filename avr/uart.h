#define BAUD(rate) (F_CPU / (rate * 16L) - 1)

/* avr-libc's putchar/getchar takes a FILE* argument
   which makes every call slightly bigger */
#undef putchar
#define putchar uart_putchar
#undef getchar
#define getchar uart_getchar

uint8_t uart_haschar(void);
void uart_init(uint16_t);
int uart_getchar(void);
int uart_putchar(char data);
int getc_timeout(uint8_t sec);
void prints(int16_t);
void printu(uint16_t);
void print(const char *);
#if 0
void printb(uint16_t number, uint8_t base);
void print32(uint32_t);
void printx(uint16_t);
#endif
