#define LASER_PIN PIND
#define LASER_DDR DDRD
#define LASER_PORT PORTD
#define LASER_EOF PORTD5 /* lcd pin 6 */
#define LASER_MOSI PORTD6 /* lcd pin 7 */
#define LASER_SCK PORTD7 /* lcd pin 8 */
#define LASER_PWR PORTD4

void laser_init(void);
void laser_on(void);
uint16_t decode(uint8_t *byte);
uint16_t laser_read(void);
