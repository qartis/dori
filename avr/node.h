#define XSTR(X) STR(X)
#define STR(X) #X

#define NODE_INIT() \
    uart_init(BAUD(38400)); \
    time_init(); \
    spi_init(); \
\
    _delay_ms(200); \
    puts_P(PSTR("\n\n" XSTR(MY_ID) " node start: " XSTR(VERSION))); \
\
\
    while (mcp2515_init()) { \
        puts_P(PSTR("mcp: init")); \
        _delay_ms(500); \
    } \
\
    sei();
