#include <FL/Fl.H>
#include "objmodel.h"
#include "FlGlArcballWindow.h"

class viewport : public FlGlArcballWindow {
  public:
    viewport(int W,int H,const char*L=0);

    virtual void draw();
    void addModel(ObjModel& model);
    
  private:
    std::vector<ObjModel> models;
};
/*

class viewport : public FLGlArcballWindow {  
public:
    viewport(int x, int y, int w, int h, const char *l);
    virtual int handle(int event);

    void addModel(ObjModel& model);

private:
    void draw();

    Vector3f deltaPosition;

};
*/
