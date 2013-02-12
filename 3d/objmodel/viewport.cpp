#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <GL/glut.h>
#include <vector>
#include <list>
#include <math.h>
#include "basic_ball.h"
#include "../../row.h"
#include "FlGlArcballWindow.h"
#include "msmvtl/tmatrix.h"
#include "objmodel.h"
#include "fl_gl_contour.h"
#include "viewport.h"

const static char* laser_type = "laser";
const static char* gps_type = "gps";
const static char* accel_type = "accel";
const static char* arm_type = "arm";
const static char* plate_type = "plate";
const static char* orientation_type = "orientation";

Viewport::Viewport(int x, int y, int w,int h,const char*l, bool dori, bool showCont) :
FlGlArcballWindow(w,h,l) {
    end();

    showDORI = dori;
    showContour = showCont;

    rowData = NULL;
    dori_body = NULL;
    dori_arm = NULL;
    dori_sensor_plate = NULL;

    contour = NULL;

    if(showDORI) {
        dori_body = new ObjModel;
        dori_body->load("../3d/objmodel/models/dori_body.obj");

        dori_arm = new ObjModel;
        dori_arm->load("../3d/objmodel/models/dori_arm.obj");

        dori_sensor_plate = new ObjModel;
        dori_sensor_plate->load("../3d/objmodel/models/dori_sensor_plate.obj");

        // line the arm up
        dori_arm->setPos(-0.72682, 0.66008, -0.37968);

        // line the plate up, set the other positions to 0
        // because we'll be borrowing the arm's glMatrix when
        // we draw the sensor plate
        dori_sensor_plate->setPos(0, 1.598, 0);
    }

    if(showContour) {
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
        data.read_file("../3d/objmodel/wafer2.xyz",0,0);
        contour->set_input_data(data);
        contour->actualize();

        graphType2d = 0;
        graphType3d = 2;

        gridXCells = 40;
        gridYCells = 40;
        gridZCells = 30;

        contour->set_number_x_grid(gridXCells);
        contour->set_number_y_grid(gridYCells);
        contour->set_number_z_grid(gridZCells);

        contour->set_submesh_limits(0,0,contour->get_x_min());
        contour->set_submesh_limits(0,1,contour->get_x_max());
        contour->set_submesh_limits(1,0,contour->get_y_min());
        contour->set_submesh_limits(1,1,contour->get_y_max());
        contour->normalize_submesh_limits();

        contour->set_average_duplicated(true);

        contour->intp_method(0);
        contour->set_palette(0);
        contour->graph_2d(graphType2d);
        contour->graph_3d(graphType3d);
        contour->graph_cb();
    }
}

Viewport::~Viewport() {
    if(showContour) {
        delete contour;
    }

    if(showDORI) {
        delete dori_body;
        delete dori_arm;
        delete dori_sensor_plate;
    }
}


void Viewport::addModel(ObjModel& model) {
    models.push_back(model);
}

ObjModel* Viewport::getModel(const char* file) {
    for(unsigned int i = 0; i < models.size(); i++) {
        if(strstr(models[i].getFilename(), file)) {
            return &models[i];
        }
    }

    return NULL;
}


int Viewport::handle(int event) {
    bool needToRedraw = false;

    switch(event) {
    case FL_KEYDOWN: {
        int key = Fl::event_key();
#if 0
        if(key == 'a') {
            dori_sensor_plate->yRot+=5;
            dori_sensor_plate->draw();
            redraw();
        }
        else if(key == 'd') {
            dori_sensor_plate->yRot-=5;
            dori_sensor_plate->draw();
            redraw();
        }
        else if(key == 'w') {
            dori_arm->xRot-=5;
            redraw();
        }
        else if(key == 's') {
            dori_arm->xRot+=5;
            redraw();
        }
        else if(key == 'h') {
            dori_body->yRot+=5;
            redraw();
        }
        else if(key == 'l') {
            dori_body->yRot-=5;
            redraw();
        }
        else if(key == 'j') {
            dori_body->xRot+=5;
            redraw();
        }
        else if(key == 'k') {
            dori_body->xRot-=5;
            redraw();
        }
        else if(key == FL_Down) {
            dori_body->xPos-=5;
            redraw();
        }
#endif


        // interpolation - 3
        // palette - 5
        // 2d graph: on/off, 4 types
        // 3d graph: on/off, 4 types
        // duplicate: avg or delete, tolerance

        //help: ?
        //pallette = 1 - 5
        //interpolation = 7 - 9
        //2d graph: y u i o p
        //3d graph: h j k l ;
        //grid x size vb, grid y size nm, grid z size ,.
        //display: z box, x palette, c data, v points
        //f open file

        if(showContour) {
            if(key >= '1' && key <= '5') {
                // these set functions (eg set_palette) return 1 if the value has changed and 0 if the value hasn't changed
                if(contour->set_palette(key - '1')) {
                    int palette = key - '1';
                    printf("setting palette to %d\n", palette);
                    needToRedraw = true;
                }
            }
            else if(key >= '7' && key <= '9') {
                if(contour->intp_method(key - '7')) {
                    printf("setting interpolation method\n");
                    needToRedraw = true;
                }
            }
            else if ((key == 'y' && contour->graph_2d(0)) ||
                     (key == 'u' && contour->graph_2d(1)) ||
                     (key == 'i' && contour->graph_2d(2)) ||
                     (key == 'o' && contour->graph_2d(3)) ||
                     (key == 'p' && contour->graph_2d(4)) ||
                     (key == 'h' && contour->graph_3d(0)) ||
                     (key == 'j' && contour->graph_3d(1)) ||
                     (key == 'k' && contour->graph_3d(2)) ||
                     (key == 'l' && contour->graph_3d(3)) ||
                     (key == ';' && contour->graph_3d(4))) {
                needToRedraw = true;
            }


            if(needToRedraw) {
                printf("redrawing..\n");
                contour->graph_cb();
                contour->redraw();
                redraw();
            }

            return 1;
        }
    }
    }
    return FlGlArcballWindow::handle(event);
}

//rewrite draw method
void Viewport::draw(){
    reshape();
    if(!valid()){
        static GLfloat light_diffuse[] = {1.0, 0.0, 0.0, 1.0};
        static GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glClearColor (0.25,0.5,0.5, 1.0);
        glDepthFunc (GL_LESS);
        glEnable (GL_DEPTH_TEST);
        glDisable (GL_DITHER);
    }

    valid(1);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    arcball_transform();//apply transformation
    draw_axis();
    //draw_zoom_scale();

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    // rotate contours to have the correct orientation
    float vAng	= 95.0;//Z
    float hAng	= 220.0;//XY
    glRotatef(hAng,0,1,0);
    glRotatef(vAng,-1,0,0);

    if(showContour) {
        contour->draw();
    }

    glPopMatrix();

    glScalef(0.1f, 0.1f, 0.1f);

    /*
    for(int i = 0; i < models.size(); i++) {
        models[i].draw();
    }
    */

    glPushMatrix();

    if(!rowData) {
        rowData = (std::vector<Row>*)user_data();
    }

    std::vector<Row>::reverse_iterator it;

    // reset rotations
    dori_body->xRot = 0;
    dori_body->yRot = 0;
    dori_body->zRot = 0;

    dori_arm->xRot = 0;
    dori_sensor_plate->yRot = 0;

    for(it = rowData->rbegin(); it != rowData->rend(); it++) {
        if(strcmp(it->cols[1], orientation_type) == 0) {
            dori_body->xRot = atof(it->cols[2]);
            dori_body->yRot = atof(it->cols[3]);
            dori_body->zRot = atof(it->cols[4]);
        }
        if(strcmp(it->cols[1], arm_type) == 0) {
            dori_arm->xRot = atof(it->cols[2]);
        }
        if(strcmp(it->cols[1], plate_type) == 0) {
            dori_sensor_plate->yRot = atof(it->cols[2]);
        }
    }

    if(showDORI) {
        dori_body->draw(false);
        dori_arm->draw(false);
        dori_sensor_plate->draw(false);
    }

    glPopMatrix();

    glEnable(GL_LIGHTING);

    draw_3d_orbit();
    glPopMatrix();
    glFinish();
}
