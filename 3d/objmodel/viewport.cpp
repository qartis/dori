#include <GL/glut.h>
#include "FL/Fl_Double_Window.H"
#include <math.h>
#include "basic_ball.h"
#include "viewport.h"

//make derived class
viewport::viewport(int W,int H,const char*L):FlGlArcballWindow(W,H,L){}

void viewport::addModel(ObjModel& model) {
    models.push_back(model);
}

//rewrite draw method
void viewport::draw(){
    reshape();
    if(!valid()){
        static GLfloat light_diffuse[] = {1.0, 0.0, 0.0, 1.0};
        static GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
    }
    valid(1);
    glClearColor (0.25,0.5,0.5, 1.0);
    glDepthFunc (GL_LESS);
    glEnable (GL_DEPTH_TEST);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    arcball_transform();//apply transformation
    draw_axis();
    draw_zoom_scale();      

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    for(int i = 0; i < models.size(); i++) {
        models[i].draw(i * 2.0, 0.0, 0.0);
    }

    draw_3d_orbit();
    glPopMatrix();
    glFinish();
};
