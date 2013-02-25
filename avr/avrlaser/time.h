extern volatile uint32_t time;

void time_init(void);

#define IS_TIME(id) (id == (MSG_VALUE_PERIODIC << 8 | TYPE_TIME))
