#include <GL/glut.h>
#include <math.h>
#include "basic_ball.h"
#include "viewport.h"
#include "msmvtl/tmatrix.h"

const static char* laser_type = "laser";
const static char* gps_type = "gps";
const static char* accel_type = "accel";
const static char* robot_filename = "robot.obj";

static void resetCallback(void *widget) {
    if(widget == NULL) {
        perror("widget = NULL in 3d reset callback");
        return;
    }

    viewport *vp = (viewport *)widget;

    if(vp == NULL) {
        printf("vp is null\n");
    }
    vp->dataList.clear();
    vp->dataIterator = vp->dataList.begin();

    vp->redraw();
}
static void dataCallback(void *data, void *widget) {
    if(widget == NULL) {
        printf("widget = NULL in 3d data callback");
        return;
    }

    if(data == NULL) {
        printf("data = NULL in 3d data callback");
        return;
    }

    viewport *vp = (viewport *)widget;
    ObjModel* robot = vp->getModel(robot_filename);

    if(robot == NULL) {
        printf("robot model not found!\n");
        return;
    }

    printf("inserting sorted data\n");
    char *buf = (char *)data;
    vp->insertDataSorted(buf);
}

static void processData(void *ptr) {
    printf("called processData...\n");

    viewport* vp = (viewport*)ptr;
    char type[32];
    char a[32];
    char b[32];
    char c[32];
    unsigned long time = 0;

    if(vp->dataIterator != vp->dataList.end()) {
        sscanf(vp->dataIterator->dataString, "%[^,],%[^,],%[^,],%[^,],%d", type, a, b, c, &time);


        if(strcmp(type, accel_type) == 0) {
            float af;
            float bf;
            float cf;

            af = atof(a);
            bf = atof(b);
            cf = atof(c);

            printf("setting rotation to: %f %f %f\n", af, bf, cf);
            vp->getModel(robot_filename)->setRot(af, bf, cf);
            vp->redraw();
        }
        ++vp->dataIterator;
    }
    else {
        vp->dataIterator = vp->dataList.begin();
    }
    Fl::repeat_timeout(1.0, processData, ptr);
}

void viewport::insertDataSorted(char *s) {
    std::list<dataPoint>::iterator it = dataList.begin();

    char type[32];
    char a[32];
    char b[32];
    char c[32];
    unsigned long time = 0;

    sscanf(s, "%[^,],%[^,],%[^,],%[^,],%d", type, a, b, c, &time);

    dataPoint dp;

    dp.time = time;
    sprintf(dp.dataString, "%s, %s, %s, %s", type, a, b, c);

    printf("adding %s to list\n", dp.dataString);
    if(dataList.size() == 0) {
        dataList.push_back(dp);
    }
    else {
        while(it != dataList.end()) {
            if(it->time < time) {
                dataList.insert(it, dp);
                break;
            }
            ++it;
        }
    }
}

//make derived class
viewport::viewport(int w,int h,const char*l) :
FlGlArcballWindow(w,h,l), tableViewWindow(w, h), tableView(0, 0, w, h) {
    // end() to prevent fl_gl_contour from being added to tableViewWindow
    end();

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

    tableViewWindow.position(x() + w, y());
    tableView.parentWidget = this;
    tableView.widgetDataCallback = dataCallback;
    tableView.widgetResetCallback = resetCallback;

    dataIterator = dataList.begin();

    Fl::add_timeout(2.0, processData, (void*)this);
}

void viewport::addModel(ObjModel& model) {
    models.push_back(model);
}

ObjModel* viewport::getModel(const char* file) {
    for(unsigned int i = 0; i < models.size(); i++) {
        if(strstr(models[i].getFilename(), file)) {
            return &models[i];
        }
    }

    return NULL;
}


int viewport::handle(int event) {
    bool needToRedraw = false;

    switch(event) {
    case FL_KEYDOWN: {
        int key = Fl::event_key();
        if(key == (FL_F + 1)) {
            if(!tableViewWindow.shown()) {
                tableViewWindow.show();
            }
            else {
                tableViewWindow.hide();
            }
            return 1;
        }
        else if(key >= '1' && key <= '5') {
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


        if(needToRedraw) {
            printf("redrawing..\n");
            contour->graph_cb();
            contour->redraw();
            redraw();
        }

        return 1;
    }
    }
    return FlGlArcballWindow::handle(event);
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

    contour->draw();
    glPopMatrix();

    glScalef(0.1f, 0.1f, 0.1f);
    for(int i = 0; i < models.size(); i++) {
        models[i].draw();
    }

    glEnable(GL_LIGHTING);

    draw_3d_orbit();
    glPopMatrix();
    glFinish();
};
