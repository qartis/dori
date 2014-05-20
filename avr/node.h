#define XSTR(X) STR(X)
#define STR(X) #X

volatile uint8_t reinit;
struct mcp2515_packet_t packet __attribute__ ((section (".noinit")));
uint8_t boot_mcusr __attribute__ ((section (".noinit")));


void sleep(void);

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
    for (;;) {
        printwait_P(PSTR("BADISR\n"));
    }
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
    _delay_ms(300); \
    uart_init(BAUD(UART_BAUD)); \
    DEBUG_INIT; \
    time_init(); \
    spi_init(); \
    set_sleep_mode(SLEEP_MODE_IDLE); \
\
    _delay_ms(300); \
    printf_P(PSTR("\n" XSTR(MY_ID) " start: " XSTR(VERSION) " ")); \
    printf_P((boot_mcusr & (1 << WDRF)) ? PSTR("wd") : PSTR("")); \
    printf_P((boot_mcusr & (1 << BORF)) ? PSTR("bo") : PSTR("")); \
    printf_P((boot_mcusr & (1 << EXTRF)) ? PSTR("ext") : PSTR("")); \
    printf_P((boot_mcusr & (1 << PORF)) ? PSTR("po") : PSTR("")); \
    printf_P((boot_mcusr & ((1 << PORF) | (1 << EXTRF) | (1 << BORF) | (1 << WDRF))) == 0 ? PSTR("software") : PSTR("")); \
    printf_P(PSTR(" reboot\n")); \
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

#ifdef USER1_IRQ
#define TRIGGER_USER1_IRQ() do { irq_signal |= IRQ_USER1; } while(0)
#define CHECK_USER1 \
        if (irq_signal & IRQ_USER1) { \
            rc = user1_irq(); \
            irq_signal &= ~IRQ_USER1; \
            if (rc) {\
                printwait_P(PSTR("$$U\n"));\
                goto reinit;\
            } \
        }
#else
#define CHECK_USER1
#endif

#ifdef USER2_IRQ
#define TRIGGER_USER2_IRQ() do { irq_signal |= IRQ_USER2; } while(0)
#define CHECK_USER2 \
        if (irq_signal & IRQ_USER2) { \
            rc = user2_irq(); \
            irq_signal &= ~IRQ_USER2; \
            if (rc) {\
                printwait_P(PSTR("$$U\n"));\
                goto reinit;\
            } \
        }
#else
#define CHECK_USER2
#endif

#ifdef USER3_IRQ
#define TRIGGER_USER3_IRQ() do { irq_signal |= IRQ_USER3; } while(0)
#define CHECK_USER3 \
        if (irq_signal & IRQ_USER3) { \
            rc = user3_irq(); \
            irq_signal &= ~IRQ_USER3; \
            if (rc) {\
                printwait_P(PSTR("$$U\n"));\
                goto reinit;\
            } \
        }
#else
#define CHECK_USER3
#endif

#define NODE_MAIN() \
    /*wdt_enable(WDTO_8S);*/ \
\
    PROMPT \
\
    for (;;) { \
        if (0 && reinit) goto reinit; \
\
        while (irq_signal == 0) { /*sleep();*/ } \
\
        CHECK_USER1; \
        CHECK_USER2; \
        CHECK_USER3; \
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
    }
