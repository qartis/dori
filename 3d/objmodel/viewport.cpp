#include "viewport.h"
#include <math.h>

viewport::viewport(int x, int y, int w, int h, const char *l)
    : model(NULL),
    Fl_Gl_Window(x, y, w, h, l) {}

int viewport::handle(int event) {
    static int originalX = 0, originalY = 0;
    static int lastX = 0, lastY = 0;

    switch(event) {
        case FL_KEYDOWN: {
            int key = Fl::event_key();
            if(key == 'w') {
                position.y -= 0.05;
                redraw();
            }
            else if(key == 's') {
                position.y += 0.05;
                redraw();
            }
            if(key == 'a') {
                position.x += 0.05;
                redraw();
            }
            else if(key == 'd') {
                position.x -= 0.05;
                redraw();
            }
        }
        break;
        case FL_PUSH: {
            originalX = x() - Fl::event_x();
            originalY = y() - Fl::event_y();
        }
        case FL_DRAG: {
            if(Fl::event_button1()) { // left click
                int deltaX = 0, deltaY = 0;
                deltaX = x() - Fl::event_x() - originalX;
                deltaY = y() - Fl::event_y() - originalY;

                position.x -= 0.01 * deltaX;
                position.y += 0.01 * deltaY;

                originalX = x() - Fl::event_x();
                originalY = y() - Fl::event_y();

                redraw();
                return 1;
            }
            else if(Fl::event_button2()) { // middle click
                rotation.y = originalX + Fl::event_x();
                rotation.x = originalY + Fl::event_y();
                redraw();
            }
            return 1;
        }
        break;
        case FL_MOUSEWHEEL: {
            position.z -= Fl::event_dy();
            redraw();
            return 1;
        }
        break;
    }
}



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

    glTranslatef(0.0 + position.x, 0.0 + position.y, -5.0 + position.z);
    glRotatef(rotation.x, 1.0, 0.0, 0.0);
    glRotatef(rotation.y, 0.0, 1.0, 0.0);
    glRotatef(rotation.z, 0.0, 0.0, 1.0);

    // Position object
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if(model != NULL)
    {
        model->draw(0.0, 0.0, 0.0);
    }
}
