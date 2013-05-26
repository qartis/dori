class Fl_Gauge : public Fl_Dial {
public:

    Fl_Gauge(int x, int y, int w, int h, const char *l = 0);

    void setScale(int _major_step_size, int _minor_step_size) {
        major_step_size = _major_step_size;
        minor_step_size = _minor_step_size;
    }

private:
    void draw(void);
    int handle(int);

    int major_step_size;
    int minor_step_size;
};
