#include <FL/Fl.H>
#include <FL/Fl_Widget.H>

class RadarWidget : public Fl_Widget {

public:
    RadarWidget(int x, int y, int w, int h, const char *label);

    void insertDataPoint(int index, float distance);

// overridden functions
    virtual int handle(int event);
    virtual void draw();


private:

    void drawBase();
    int data[100];
    float insideAngle;
};
