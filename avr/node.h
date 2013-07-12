#define XSTR(X) STR(X)
#define STR(X) #X

#ifdef DEBUG
#define DEBUG_INIT debug_init();
#else
#define DEBUG_INIT
#endif

#define PROMPT \
    printf_P(PSTR("\n" XSTR(MY_ID) "> "));

#define NODE_INIT() \
    uint8_t rc; \
    wdt_disable(); \
    uart_init(BAUD(38400)); \
    DEBUG_INIT; \
    time_init(); \
    spi_init(); \
\
    _delay_ms(300); \
    puts_P(PSTR("\n" XSTR(MY_ID) " start: " XSTR(VERSION))); \
\
\
    goto reinit; \
\
reinit: \
    cli(); \
    while (mcp2515_init()) { \
        puts_P(PSTR("mcp: init")); \
        _delay_ms(500); \
    } \
\
    rc = 0;



#define NODE_MAIN() \
    PROMPT \
\
    for (;;) { \
        while (irq_signal == 0) {}; \
\
        CHECK_IRQS; \
    }

#ifdef DEBUG
#define CHECK_IRQS \
        CHECK_CAN; \
        CHECK_TIMER; \
        CHECK_UART; \
        CHECK_DEBUG;
#else
#define CHECK_IRQS \
        CHECK_CAN; \
        CHECK_TIMER; \
        CHECK_UART;
#endif

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
        if (irq_signal & IRQ_TIMER) { \
            rc = periodic_irq(); \
            irq_signal &= ~IRQ_TIMER; \
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

#define CHECK_DEBUG \
        if (irq_signal & IRQ_DEBUG) { \
            rc = debug_irq(); \
            irq_signal &= ~IRQ_DEBUG; \
        } \
        if (rc) {\
            puts_P(PSTR("$$4"));\
            goto reinit;\
        }
