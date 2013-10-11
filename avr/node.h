#define XSTR(X) STR(X)
#define STR(X) #X

uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

void get_mcusr(void) __attribute__((section(".init3"), naked, used));
void get_mcusr(void)
{
    mcusr_mirror = MCUSR;
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
    printf_P((mcusr_mirror & (1 << WDRF)) ? PSTR("wd") : PSTR("")); \
    printf_P((mcusr_mirror & (1 << BORF)) ? PSTR("bo") : PSTR("")); \
    printf_P((mcusr_mirror & (1 << EXTRF)) ? PSTR("ext") : PSTR("")); \
    printf_P((mcusr_mirror & (1 << PORF)) ? PSTR("po") : PSTR("")); \
    puts_P(PSTR(" reboot")); \
\
\
    goto reinit; \
\
reinit: \
    _delay_ms(100); \
    cli(); \
    while (mcp2515_init()) { \
        puts_P(PSTR("mcp: init")); \
        _delay_ms(500); \
    } \
\
    rc = 0;


#define CHECK_CAN \
        if (irq_signal & IRQ_CAN) { \
            rc = can_irq(); \
            irq_signal &= ~IRQ_CAN; \
        } \
        if (rc) {\
            puts_P(PSTR("$$1"));\
            goto reinit;\
        }

#define CHECK_TIMER \
        if (irq_signal & IRQ_PERIODIC) { \
            rc = periodic_irq(); \
            irq_signal &= ~IRQ_PERIODIC; \
        } \
        if (rc) {\
            puts_P(PSTR("$$2"));\
            goto reinit;\
        }\

#define CHECK_UART \
        if (irq_signal & IRQ_UART) { \
            rc = uart_irq(); \
            irq_signal &= ~IRQ_UART; \
        } \
        if (rc) {\
            puts_P(PSTR("$$3"));\
            goto reinit;\
        }

#ifdef DEBUG
#define CHECK_DEBUG \
        if (irq_signal & IRQ_DEBUG) { \
            rc = debug_irq(); \
            irq_signal &= ~IRQ_DEBUG; \
        } \
        if (rc) {\
            puts_P(PSTR("$$4"));\
            goto reinit;\
        }
#else
#define CHECK_DEBUG
#endif

#ifdef USER_IRQ
#define CHECK_USER \
        if (irq_signal & IRQ_USER) { \
            rc = user_irq(); \
            irq_signal &= ~IRQ_USER; \
        } \
        if (rc) {\
            puts_P(PSTR("$$5"));\
            goto reinit;\
        }
#else
#define CHECK_USER
#endif


#define NODE_MAIN() \
    PROMPT \
\
    for (;;) { \
        while (irq_signal == 0) {}; \
\
        CHECK_CAN; \
        CHECK_TIMER; \
        CHECK_UART; \
        CHECK_DEBUG; \
        CHECK_USER; \
    }
