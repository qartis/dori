#define XSTR(X) STR(X)
#define STR(X) #X

extern volatile uint8_t reinit;
extern struct mcp2515_packet_t packet;
extern uint8_t boot_mcusr;

void sleep(void);

void node_init(void);
uint8_t node_main(void);
uint8_t node_debug_common(const char *cmd);

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

#ifdef DEBUG
#define CHECK_DEBUG \
        if (irq_signal & IRQ_DEBUG) { \
            rc = debug_irq(); \
            irq_signal &= ~IRQ_DEBUG; \
            PROMPT;\
            if (rc) {\
                printwait_P(PSTR("$$d\n"));\
                return rc;\
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
                return rc;\
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
                return rc;\
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
                return rc;\
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
                return rc;\
            } \
        }
#else
#define CHECK_USER3
#endif
