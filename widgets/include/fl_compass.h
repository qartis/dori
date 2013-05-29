class Fl_Compass : public Fl_Dial {
public:
    Fl_Compass(int x, int y, int w, int h, const char *l = 0);

private:
    void draw(void);
    int handle(int);
};
