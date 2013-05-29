class Fl_Sparkline : public Fl_Widget {
public:

    Fl_Sparkline(int x, int y, int w, int h, const char *l = 0);

    void setValues(int *_values, int _num_values);

private:
    void draw(void);
    void drawCursor(void);
    void drawPeaks(void);
    void drawPoint(int x);
    int handle(int);

    int *values;
    int num_values;

    int max_index;
    int min_index;

    int peak_indices[128];
    int num_peaks;
    int trough_indices[128];
    int num_troughs;

    int padding;
    int width;
    int height;
};
