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
    struct DataPoint {
        bool valid;
        bool changed;
        float screenX;
        float screenY;
        float distance;
    };

    bool completeRedraw;

    void drawBase();
    struct DataPoint data[360];
    float insideAngle;

    float originX;
    float originY;
};
