struct nunchuck {
    uint16_t accel_x;
    uint16_t accel_y;
    uint16_t accel_z;
    uint8_t joy_x;
    uint8_t joy_y;
    uint8_t z_button : 1;
    uint8_t c_button : 1;
};
 
uint8_t nunchuck_init(void);
uint8_t nunchuck_read(struct nunchuck *);
