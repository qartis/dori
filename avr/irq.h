enum {
    IRQ_CAN      = (1 << 0),
    IRQ_PERIODIC = (1 << 1),
    IRQ_UART     = (1 << 2),
    IRQ_DEBUG    = (1 << 3),
    IRQ_ALARM    = (1 << 4),
    IRQ_USER     = (1 << 5),
};

extern volatile uint8_t irq_signal;
