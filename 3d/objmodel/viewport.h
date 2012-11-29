#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>
#include <GL/glu.h>
#include "objmodel.h"

class viewport : public Fl_Gl_Window {

public:
    viewport(int x, int y, int w, int h, const char *l);

    ObjModel *model;

private:
    void draw();
};
