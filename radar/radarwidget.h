#include <FL/Fl.H>
#include <FL/Fl_Widget.H>

#define MAX_ANGLES 360

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

    typedef enum {
        UPWARD,
        DOWNWARD
    } direction ;

    direction sweepDirection;

    bool completeRedraw;

    void drawBase();
    struct DataPoint data[MAX_ANGLES];
    float insideAngle;

    float originX;
    float originY;
    float scale;

    int curPointIndex;
};
