#define XSTR(X) STR(X)
#define STR(X) #X

volatile uint8_t reinit;

void sleep(void);

uint8_t boot_mcusr __attribute__ ((section (".noinit")));

void get_mcusr(void) __attribute__((section(".init3"), naked, used));
void get_mcusr(void)
{
    boot_mcusr = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

ISR(BADISR_vect)
{
    uart_tx_empty();
    printwait_P(PSTR("BADISR\n"));
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

#define NODE_INIT() \
    uint8_t rc; \
    _delay_ms(1000); \
    uart_init(BAUD(UART_BAUD)); \
    DEBUG_INIT; \
    time_init(); \
    spi_init(); \
    set_sleep_mode(SLEEP_MODE_IDLE); \
\
    _delay_ms(300); \
    printwait_P(PSTR("\n" XSTR(MY_ID) " start: " XSTR(VERSION) " ")); \
    printwait_P((boot_mcusr & (1 << WDRF)) ? PSTR("wd") : PSTR("")); \
    printwait_P((boot_mcusr & (1 << BORF)) ? PSTR("bo") : PSTR("")); \
    printwait_P((boot_mcusr & (1 << EXTRF)) ? PSTR("ext") : PSTR("")); \
    printwait_P((boot_mcusr & (1 << PORF)) ? PSTR("po") : PSTR("")); \
    printwait_P((boot_mcusr & ((1 << PORF) | (1 << EXTRF) | (1 << BORF) | (1 << WDRF))) == 0 ? PSTR("software") : PSTR("")); \
    printwait_P(PSTR(" reboot\n")); \
\
\
    goto reinit; \
\
reinit: \
    uart_tx_flush(); \
    _delay_ms(100); \
    cli(); \
    reinit = 0; \
    while (mcp2515_init()) { \
        printwait_P(PSTR("mcp: init\n")); \
        _delay_ms(1500); \
    } \
\
    rc = 0;

#ifdef DEBUG
#define CHECK_DEBUG \
        if (irq_signal & IRQ_DEBUG) { \
            rc = debug_irq(); \
            irq_signal &= ~IRQ_DEBUG; \
            PROMPT;\
            if (rc) {\
                printwait_P(PSTR("$$d\n"));\
                goto reinit;\
            } \
        }
#else
#define CHECK_DEBUG
#endif

#ifndef UART_CUSTOM_INTERRUPT
#define CHECK_UART \
        if (irq_signal & IRQ_UART) { \
            rc = uart_irq(); \
            irq_signal &= ~IRQ_UART; \
            if (rc) {\
                printwait_P(PSTR("$$u\n"));\
                goto reinit;\
            } \
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
            if (rc) {\
                printwait_P(PSTR("$$U\n"));\
                goto reinit;\
            } \
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
        while (irq_signal == 0) { sleep(); } \
\
        if (irq_signal & IRQ_CAN) { \
            rc = can_irq(); \
            irq_signal &= ~IRQ_CAN; \
            if (rc) {\
                printwait_P(PSTR("$$c\n"));\
                goto reinit; \
            } \
        } \
\
        if (irq_signal & IRQ_PERIODIC) { \
            rc = periodic_irq(); \
            irq_signal &= ~IRQ_PERIODIC; \
            if (rc) {\
                printwait_P(PSTR("$$p\n"));\
                goto reinit; \
            }\
        } \
\
        CHECK_UART; \
        CHECK_DEBUG; \
        CHECK_USER; \
    }
