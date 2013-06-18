struct hmc5883_t {
    int16_t x;
    int16_t y;
    int16_t z;
};

uint8_t hmc_init(void);
uint8_t hmc_read(struct hmc5883_t *);
void hmc_set_continuous(void);
