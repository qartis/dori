#define XSTR(X) STR(X)
#define STR(X) #X

#define NODE_INIT() \
    uint8_t rc; \
    wdt_disable(); \
    uart_init(BAUD(38400)); \
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
    for (;;) { \
        print_P(PSTR("\n" XSTR(MY_ID) "> ")); \
\
        while (irq_signal == 0) {}; \
\
        if (irq_signal & IRQ_CAN) { \
            rc = can_irq(); \
            irq_signal &= ~IRQ_CAN; \
        } \
\
        if (rc) {\
            puts_P(PSTR("$$1"));\
            goto reinit;\
        }\
\
        if (irq_signal & IRQ_TIMER) { \
            rc = periodic_irq(); \
            irq_signal &= ~IRQ_TIMER; \
        } \
\
        if (rc) {\
            puts_P(PSTR("$$2"));\
            goto reinit;\
        }\
\
        if (irq_signal & IRQ_UART) { \
            rc = uart_irq(); \
            irq_signal &= ~IRQ_UART; \
        } \
\
        if (rc) {\
            puts_P(PSTR("$$3"));\
            goto reinit;\
        }\
    }
