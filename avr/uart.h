#define BAUD(rate) (F_CPU / (rate * 16L) - 1)

#define UART_BUF_SIZE		128

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
int getc_wait(volatile uint8_t *signal);
void uart_getbuf(char *dest);
char *getline(char *s, int bufsize, FILE *f, volatile uint8_t *signal);
void prints(int16_t);
void printu(uint16_t);
void print(const char *);
void print_P(const char * PROGMEM);
void printx(uint16_t);
#if 0
void printb(uint16_t number, uint8_t base);
void print32(uint32_t);
#endif
