#define LED_DDR  DDRB
#define LED_PORT PORTB
#define LED_PIN  PINB
#define LED_BIT  (1<<PB0)

void led_on(void);
void led_off(void);
void led_toggle(void);
void led_init(void);
uint8_t led_status(void);
