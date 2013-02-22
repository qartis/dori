#define BAUD(rate) (F_CPU / (rate * 16L) - 1)

void uart_init(uint16_t) BOOTLOADER_SECTION;
int uart_getchar(void) BOOTLOADER_SECTION; 
int uart_putchar(char data) BOOTLOADER_SECTION;
void uart_print(const char *s) BOOTLOADER_SECTION;
void uart_printint(uint16_t) BOOTLOADER_SECTION;
