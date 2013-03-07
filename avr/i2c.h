#define I2C_READ    1
#define I2C_WRITE   0

#define I2C_ACK     1
#define I2C_NOACK   0

/* ((F_CPU/FREQ)-16)/2 must be >= 10 */
#define I2C_FREQ(FREQ) (((F_CPU/FREQ)-16)/2)

void i2c_init(uint8_t clk);
void i2c_stop(void);
uint8_t i2c_start(uint8_t addr);
uint8_t i2c_write(uint8_t data);
uint8_t i2c_read(uint8_t ack);
