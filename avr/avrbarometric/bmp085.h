struct bmp085_sample {
    int32_t temp;
    int32_t pressure;
};

void bmp085_get_cal_param(void);
struct bmp085_sample bmp085_read(void);
