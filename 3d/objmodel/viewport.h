#include <FL/Fl.H>
#include "objmodel.h"
#include "FlGlArcballWindow.h"
#include "fl_gl_contour.h"

class viewport : public FlGlArcballWindow {
  public:
    viewport(int W,int H,const char*L=0);

    virtual void draw();
    void addModel(ObjModel& model);

  private:
    virtual int handle(int event);

    std::vector<ObjModel> models;
    fl_gl_contour *contour;
    int graphType2d;
    int graphType3d;

    int gridXCells;
    int gridYCells;
    int gridZCells;
};
