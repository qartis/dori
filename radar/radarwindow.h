#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>
#include "../spawnablewindow.h"
#include <map>
#include <vector>

#define MAX_ANGLES 360

class RadarWindow : public SpawnableWindow {

public:
    RadarWindow(int x, int y, int w, int h, const char *label = NULL);

    void insertDataPoint(int index, float distance);

    // overridden functions
    virtual int handle(int event);
    virtual void draw();

    struct DataPoint {
        int screenX;
        int screenY;
    };

    std::vector<Row>* rowdata;


private:

    bool lookup(int rowid, DataPoint &data);
    void computeCoords(std::vector<Row>::iterator it, int &screenX, int &screenY);
    int rowidOfClosestPoint(int mouseX, int mouseY);

    bool completeRedraw;

    float insideAngle;

    float originX;
    float originY;
    float radius;
    float scale;

    int curPointRowID;
    std::map<int, DataPoint> pointCache;
};
