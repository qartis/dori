#include <GL/glut.h>
#include "FL/Fl_Double_Window.H"
#include <math.h>
#include "basic_ball.h"
#include "viewport.h"
#include "msmvtl/tmatrix.h"

//make derived class
viewport::viewport(int W,int H,const char*L): 
    FlGlArcballWindow(W,H,L){
        contour = new fl_gl_contour(172, 3, 593, 472, "no opengl");
        contour->box(FL_THIN_DOWN_BOX);
        contour->color(FL_BACKGROUND_COLOR);
        contour->selection_color(FL_BACKGROUND_COLOR);
        contour->labeltype(FL_NORMAL_LABEL);
        contour->labelfont(0);
        contour->labelsize(14);
        contour->labelcolor(FL_FOREGROUND_COLOR);
        contour->align(Fl_Align(FL_ALIGN_CENTER));
        contour->when(FL_WHEN_RELEASE);

        TMatrix<gm_real> data;
        data.clear();
        data.read_file("wafer2.xyz",0,0);
        contour->set_input_data(data);

        contour->set_number_x_grid(40);
        contour->set_number_y_grid(40);
        contour->set_number_z_grid(30);

        contour->intp_method(0);
        contour->graph_3d(3);
        contour->graph_2d(0);
        contour->set_palette(3);
        contour->graph_cb();

        /*
        printf("initializing data\n");
        contour->initialize_data();
        printf("performing interpolation\n");
        contour->lineal_interpolation();
        printf("evaluating color map\n");
        contour->eval_color_map();
        */
    }

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

    glPushMatrix();
    float vAng	= 95.0;//Z
    float hAng	= 220.0;//XY
    glRotatef(hAng,0,1,0); 
    glRotatef(vAng,-1,0,0);

    contour->draw();

    glScalef(0.1f, 0.1f, 0.1f);
    for(int i = 0; i < models.size(); i++) {
        models[i].draw(i * 2.0, 0.0, 0.0);
    }
    glPopMatrix();

    glEnable(GL_LIGHTING);

    draw_3d_orbit();
    glPopMatrix();
    glFinish();
};
