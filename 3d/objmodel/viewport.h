#include <FL/Fl.H>
#include "objmodel.h"
#include "FlGlArcballWindow.h"
#include "fl_gl_contour.h"
#include <FL/Fl_Window.H>
#include <list>
#include "../../tableview/tableview.h"

struct dataPoint {
    unsigned long time;
    char dataString[256];
};

class viewport : public FlGlArcballWindow {
public:
    viewport(int W, int H, const char*L=0);

    virtual void draw();
    void addModel(ObjModel& model);
    ObjModel* getModel(const char *filename);

    void insertDataSorted(char * s);
    std::list<dataPoint> dataList;
    std::list<dataPoint>::iterator dataIterator;

private:
    virtual int handle(int event);

    std::vector<ObjModel> models;
    fl_gl_contour *contour;
    int graphType2d;
    int graphType3d;

    int gridXCells;
    int gridYCells;
    int gridZCells;

    int robotModelIndex;

    Fl_Window tableViewWindow;
    TableView tableView;

};
