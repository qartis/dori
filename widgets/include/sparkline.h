class TipWin;

class Fl_Sparkline : public Fl_Widget {
public:

    Fl_Sparkline(int x, int y, int w, int h, const char *l = 0);

    void setValues(float *_values, int _num_values);

    void *table;
    void (*scrollFunc)(int index, void *obj);

private:
    void draw(void);
    void drawCursor(void);
    void hideCursor(void);
    void drawPeaks(void);
    void drawPoint(int x);
    int snap(int index);
    int handle(int);

    float *values;
    int num_values;

    int max_index;
    int min_index;

    int peak_indices[10000];
    int num_peaks;
    int trough_indices[10000];
    int num_troughs;

    int padding;
    int width;
    int height;

    int prev_x;

    TipWin *tip;
};
