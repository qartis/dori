#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>
#include <GL/glu.h>
#include "objmodel.h"

class viewport : public Fl_Gl_Window {

public:
    viewport(int x, int y, int w, int h, const char *l);
    virtual int handle(int event);

    void addModel(ObjModel& model);

private:
    void draw();

    Vector3f deltaPosition;

    std::vector<ObjModel> models;
};
