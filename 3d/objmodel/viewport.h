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
    std::vector<ObjModel> models;

    fl_gl_contour *contour;
};
