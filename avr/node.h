#define XSTR(X) STR(X)
#define STR(X) #X

volatile uint8_t reinit;

uint8_t boot_mcusr __attribute__ ((section (".noinit")));

void get_mcusr(void) __attribute__((section(".init3"), naked, used));
void get_mcusr(void)
{
    boot_mcusr = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

#ifdef DEBUG
#define DEBUG_INIT debug_init();
#else
#define DEBUG_INIT
#endif

#ifndef UART_BAUD
#define UART_BAUD 38400
#endif

#define PROMPT \
    putchar('>');
//    printf_P(PSTR("\n" XSTR(MY_ID) "> "));

#define NODE_INIT() \
    uint8_t rc; \
    uart_init(BAUD(UART_BAUD)); \
    DEBUG_INIT; \
    time_init(); \
    spi_init(); \
\
    _delay_ms(300); \
    puts_P(PSTR("\n" XSTR(MY_ID) " start: " XSTR(VERSION))); \
    printf_P((boot_mcusr & (1 << WDRF)) ? PSTR("wd") : PSTR("")); \
    printf_P((boot_mcusr & (1 << BORF)) ? PSTR("bo") : PSTR("")); \
    printf_P((boot_mcusr & (1 << EXTRF)) ? PSTR("ext") : PSTR("")); \
    printf_P((boot_mcusr & (1 << PORF)) ? PSTR("po") : PSTR("")); \
    puts_P(PSTR(" reboot")); \
\
\
    goto reinit; \
\
reinit: \
    _delay_ms(100); \
    cli(); \
    reinit = 0; \
    while (mcp2515_init()) { \
        puts_P(PSTR("mcp: init")); \
        _delay_ms(500); \
    } \
\
    rc = 0;

#ifdef DEBUG
#define CHECK_DEBUG \
        if (irq_signal & IRQ_DEBUG) { \
            rc = debug_irq(); \
            irq_signal &= ~IRQ_DEBUG; \
        } \
        if (rc) {\
            puts_P(PSTR("$$d"));\
            goto reinit;\
        }
#else
#define CHECK_DEBUG
#endif

#ifndef UART_CUSTOM_INTERRUPT
#define CHECK_UART \
        if (irq_signal & IRQ_UART) { \
            rc = uart_irq(); \
            irq_signal &= ~IRQ_UART; \
        } \
        if (rc) {\
            puts_P(PSTR("$$u"));\
            goto reinit;\
        }
#else
#define CHECK_UART
#endif

#ifdef USER_IRQ
#define TRIGGER_USER_IRQ() do { irq_signal |= IRQ_USER; } while(0)
#define CHECK_USER \
        if (irq_signal & IRQ_USER) { \
            rc = user_irq(); \
            irq_signal &= ~IRQ_USER; \
        } \
        if (rc) {\
            puts_P(PSTR("$$U"));\
            goto reinit;\
        }
#else
#define CHECK_USER
#endif


#define NODE_MAIN() \
    PROMPT \
\
    for (;;) { \
        if (0 && reinit) goto reinit; \
\
        while (irq_signal == 0) {}; \
\
        if (irq_signal & IRQ_CAN) { \
            rc = can_irq(); \
            irq_signal &= ~IRQ_CAN; \
        } \
        if (rc) {\
            puts_P(PSTR("$$c"));\
            goto reinit;\
        } \
\
        if (irq_signal & IRQ_PERIODIC) { \
            rc = periodic_irq(); \
            irq_signal &= ~IRQ_PERIODIC; \
        } \
        if (rc) {\
            puts_P(PSTR("$$p"));\
            goto reinit;\
        }\
\
        CHECK_UART; \
        CHECK_DEBUG; \
        CHECK_USER; \
    }
