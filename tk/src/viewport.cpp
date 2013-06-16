#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Table_Row.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/fl_draw.H>
#include <GL/glut.h>
#include <vector>
#include <list>
#include <math.h>
#include <limits.h>
#include <sqlite3.h>
#include "siteobject.h"
#include "lineobject.h"
#include "rectobject.h"
//#include "circleobject.h"
#include "colorchooser.h"
#include "toolbar.h"
#include "3dcontrols.h"
#include "basic_ball.h"
#include "row.h"
#include "queryinput.h"
#include "sparkline.h"
#include "table.h"
#include "objmodel.h"
#include "FlGlArcballWindow.h"
#include "viewport.h"

// const static char* laser_type = "laser";
// const static char* gps_type = "gps";
// const static char* accel_type = "accel";
const static char* arm_type = "arm";
const static char* plate_type = "plate";
const static char* orientation_type = "orientation";

int Viewport::sqlite_cb(void *arg, int ncols, char**cols, char **colNames) {
    (void)ncols;
    (void)colNames;
    std::vector<SiteObject*>* objs = (std::vector<SiteObject*>*)arg;

    int rowid = atoi(cols[0]);
    SiteObjectType type = (SiteObjectType)(atoi(cols[1]));

    SiteObject *obj;

    /*
    switch(type) {
    case LINE:
        obj = new LineObject;
        break;
    case RECT:
        obj = new RectObject;
        break;
    case CIRCLE:
        obj = new CircleObject;
        break;
    default:
        fprintf(stderr, "ERROR: undefined object type\n");
    }

    obj->fromString(cols[2]);
    obj->id = rowid;

    objs->push_back(obj);
    */

    return 0;
}

Viewport::Viewport(int x, int y, int w,int h,const char*l, bool dori, bool showCont) :
FlGlArcballWindow(x, y, w,h,l) {
    end();

    showDORI = dori;
    showContour = showCont;

    user_data(NULL);
    table = NULL;
    dori_body = NULL;
    dori_arm = NULL;
    dori_sensor_plate = NULL;
    db = NULL;
    midX = midY = 0.0;

    if(showDORI) {
        dori_body = new ObjModel;
        dori_body->load("data/dori_body.obj");

        dori_arm = new ObjModel;
        dori_arm->load("data/dori_arm.obj");

        dori_sensor_plate = new ObjModel;
        dori_sensor_plate->load("data/dori_sensor_plate.obj");

        // line the arm up
        dori_arm->setPos(-0.72682, 0.66008, -0.37968);

        // line the plate up, set the other positions to 0
        // because we'll be borrowing the arm's glMatrix when
        // we draw the sensor plate
        dori_sensor_plate->setPos(0, 1.598, 0);
    }

    if(showContour) {
        FILE *f = fopen("data/poland.clean.xyz", "r");

        char buf[512];

        if(f == NULL) {
            printf("file not found\n");
            return;
        }

        float curX, curY, curElevation;

        minX = minY = minElevation = INT_MAX;
        maxX = maxY = maxElevation = 0;

        /*
        while(fgets(buf, sizeof(buf), f)) {
            if(buf[0] == '#') {
                continue;
            }
            sscanf(buf, "%f %f %f", &curX, &curY, &curElevation);

            RectObject *newPoint = new RectObject;
            newPoint->worldLeft = curX / 5.0;
            newPoint->worldTop = curY / 5.0;
            newPoint->worldWidth = 0.02;
            newPoint->worldLength = 0.02;
            newPoint->elevation = curElevation / 1000.0;
            newPoint->worldHeight = 0.02;

            if(newPoint->worldLeft < minX) minX = newPoint->worldLeft;
            if(newPoint->worldTop < minY) minY = newPoint->worldTop;

            if(newPoint->worldLeft > maxX) maxX = newPoint->worldLeft;
            if(newPoint->worldTop > maxY) maxY = newPoint->worldTop;

            if(newPoint->elevation < minElevation) minElevation = newPoint->elevation;
            if(newPoint->elevation > maxElevation) maxElevation = newPoint->elevation;

            points.push_back(newPoint);
        }

        midX = minX + ((maxX - minX) / 2.0);
        midY = minY + ((maxY - minY) / 2.0);

        scaleColour = 6.0 / (maxElevation - minElevation);

        for(unsigned i = 0; i < points.size(); i++) {
            double r, g, b;
            RectObject *point = (RectObject*)points[i];
            Fl_Color_Chooser::hsv2rgb(scaleColour * (points[i]->elevation - minElevation), 1.0, 0.6, r, g, b);

            point->r = r * 255.0;
            point->g = g * 255.0;
            point->b = b * 255.0;

            points[i]->worldLeft -= midX;
            // invert Y
            points[i]->worldTop = maxY - points[i]->worldTop + minY - midY;
        }

        fprintf(stderr, "opening db\n");
        sqlite3_open("../siteobjects.db", &db);
        if(db) {
            const char* query = "SELECT rowid, * FROM objects;";
            int ret = sqlite3_exec(db, query, sqlite_cb, &siteObjects, NULL);
            if (ret != SQLITE_OK)
            {
                fprintf(stderr, "error executing query in viewport\n");
            }
        }

        for(unsigned i = 0; i < siteObjects.size(); i++) {
            siteObjects[i]->scaleWorldCoords(midX, minY, midY, maxY);
        }

        redraw();
        */
    }
}

Viewport::~Viewport() {
    if(showContour) {
        //delete contour;
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
    //bool needToRedraw = false;

    switch(event) {
    case FL_KEYDOWN: {
        //int key = Fl::event_key();

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
        //

        if(showContour) {
            /*
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
                return 1;
            }
            */
        }
    }
    }
    return FlGlArcballWindow::handle(event);
}

//rewrite draw method
void Viewport::draw() {
    reshape();
    if(!valid()){
        static GLfloat light_diffuse[] = {1.0, 0.0, 0.0, 1.0};
        static GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        //glClearColor (0.25,0.5,0.5, 1.0);
        glClearColor (0.1,0.1,0.1, 1.0);
        glDepthFunc (GL_LESS);
        glEnable (GL_DEPTH_TEST);
        glDisable (GL_DITHER);
    }

    valid(1);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    arcball_transform();//apply transformation
    draw_axis();

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(showContour) {
        /*
        // rotate contours to have the correct orientation
        float vAng	= 95.0;//Z
        float hAng	= 220.0;//XY
        glRotatef(hAng,0,1,0);
        glRotatef(vAng,-1,0,0);

        contour->draw();
        */

        for(unsigned i = 0; i < points.size(); i++) {
            //switch my points to RectObjects and draw them properly
            points[i]->drawWorld();
        }

        for(unsigned i = 0; i < siteObjects.size(); i++) {
            siteObjects[i]->drawWorld();
        }
    }


    glPushMatrix();

    glScalef(0.012f, 0.012f, 0.012f);

    if(showDORI) {
        if(table) {
            std::vector<Row>::reverse_iterator it;

            // reset rotations
            dori_body->xRot = 0;
            dori_body->yRot = 0;
            dori_body->zRot = 0;

            dori_arm->xRot = 0;
            dori_sensor_plate->yRot = 0;

            int typeCol, aCol, bCol, cCol;
            typeCol = aCol = bCol = cCol = -1;

            for(std::vector<Row>::iterator it = table->rowdata.begin(); it != table->rowdata.end(); it++) {
                for(unsigned i = 0; i < table->headers->size(); i++) {
                    /*if(strncmp((*(table->headers))[i], "row", strlen("row")) == 0) {
                        rowidCol = i;
                    }
                    else */
                    if(strcmp((*(table->headers))[i], "type") == 0) {
                        typeCol = i;
                    }
                    else if(strcmp((*(table->headers))[i], "a") == 0) {
                        aCol = i;
                    }
                    else if(strcmp((*(table->headers))[i], "b") == 0) {
                        bCol = i;
                    }
                    else if(strcmp((*(table->headers))[i], "c") == 0) {
                        cCol = i;
                    }
                }

                if(typeCol != -1) {
                    if(strcmp(it->cols[typeCol], orientation_type) == 0) {
                        if(aCol != -1) {
                            dori_body->xRot = atof(it->cols[aCol]);
                        }

                        if(bCol !=- 1) {
                            dori_body->yRot = atof(it->cols[bCol]);
                        }

                        if(cCol != -1) {
                            dori_body->zRot = atof(it->cols[cCol]);
                        }
                    }
                    if(strcmp(it->cols[typeCol], arm_type) == 0) {
                        if(aCol != -1) {
                            dori_arm->xRot = atof(it->cols[aCol]);
                        }
                    }
                    if(strcmp(it->cols[typeCol], plate_type) == 0) {
                        if(aCol != -1) {
                            dori_sensor_plate->yRot = atof(it->cols[aCol]);
                        }
                    }
                }
            }
        }
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
