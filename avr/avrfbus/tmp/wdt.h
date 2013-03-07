#define wdt_set(value)   \
    __asm__ __volatile__ (  \
        "in __tmp_reg__,__SREG__" "\n\t"    \
        "cli" "\n\t"    \
        "wdr" "\n\t"    \
        "sts %0,%1" "\n\t"  \
        "out __SREG__,__tmp_reg__" "\n\t"   \
        "sts %0,%2" \
        : /* no outputs */  \
        : "M" (_SFR_MEM_ADDR(_WD_CONTROL_REG)), \
        "r" (_BV(_WD_CHANGE_BIT) | _BV(WDE)), \
            "r" ((uint8_t) ((value & 0x08 ? _WD_PS3_MASK : 0x00) | \
                _BV(WDIE) | (value & 0x07)) ) \
        : "r0"  \
    )



#define wdt_off(x)   \
    __asm__ __volatile__ (  \
        "in __tmp_reg__,__SREG__" "\n\t"    \
        "cli" "\n\t"    \
        "wdr" "\n\t"    \
        "sts %0,%1" "\n\t"  \
        "sts %0,%2" \
        : /* no outputs */  \
        : "M" (_SFR_MEM_ADDR(_WD_CONTROL_REG)), \
        "r" (_BV(_WD_CHANGE_BIT) | _BV(WDE)), \
            "r" ((uint8_t) (0x00)) \
        : "r0"  \
    )
