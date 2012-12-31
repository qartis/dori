#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>
#include "../tableview/tableview.h"

#define MAX_ANGLES 360

class RadarWidget : public Fl_Widget {

public:
    RadarWidget(int x, int y, int w, int h, const char *label);

    void insertDataPoint(int index, float distance);

    // overridden functions
    virtual int handle(int event);
    virtual void draw();

    struct DataPoint {
        bool valid;
        bool changed;
        float screenX;
        float screenY;
        float distance;
    };

    struct DataPoint data[MAX_ANGLES];

private:
    bool completeRedraw;

    float insideAngle;

    float originX;
    float originY;
    float scale;

    int curPointIndex;

    Fl_Window tableViewWindow;
    TableView tableView;
};
