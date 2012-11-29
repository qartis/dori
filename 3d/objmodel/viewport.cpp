#include "viewport.h"
#include <math.h>

viewport::viewport(int x, int y, int w, int h, const char *l)
    : model(NULL),
    Fl_Gl_Window(x, y, w, h, l) {}

void viewport::draw() {
    if (!valid()) {
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClearDepth(1.0);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);
        glShadeModel(GL_FLAT);
    }

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    // Position camera/viewport init
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0,0,w(),h());
    gluPerspective(45.0, (float)w()/(float)h(), 1.0, 10.0);
    glTranslatef(0.0, 0.0, -10.0);
    // Position object
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    static float rotation = 0.0;

    glRotatef(rotation++, 1.0, 0.0, 0.0);
    glRotatef(20, 0.0, 1.0, 0.0);

    if(model != NULL)
    {
        model->draw(0.0, 0.0, 0.0);
    }
}

