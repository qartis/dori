enum {
    IRQ_CAN      = (1 << 0),
    IRQ_PERIODIC = (1 << 1),
    IRQ_UART     = (1 << 2),
    IRQ_DEBUG    = (1 << 3),
    IRQ_SECONDS  = (1 << 4),
    IRQ_USER1    = (1 << 5),
    IRQ_USER2    = (1 << 6),
    IRQ_USER3    = (1 << 7),
};

extern volatile uint8_t irq_signal;

uint8_t user1_irq(void);
uint8_t user2_irq(void);
uint8_t user3_irq(void);
uint8_t can_irq(void);
uint8_t debug_irq(void);
uint8_t uart_irq(void);
uint8_t periodic_irq(void);

