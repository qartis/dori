#include "viewport.h"
#include <GL/glut.h>
#include "arcball.h"
#include <math.h>

vec world( 0.0f, 0.0f, -20.0f );
vec eye( 0.0f, 0.0f, -20.0f );
vec centre( 0.0f, 0.0f, 0.0f );
const vec up( 0.0f, 1.0f, 0.0f );
float SPHERE_RADIUS = 5.0f;
const int SPHERE_LAT_SLICES = 12;
const int SPHERE_LONG_SLICES = 24;
const float PI = 3.141592654f;

inline vec rotate_x( vec v, float sin_ang, float cos_ang )
{
	return vec(
	    v.x,
	    (v.y * cos_ang) + (v.z * sin_ang),
	    (v.z * cos_ang) - (v.y * sin_ang)
	    );
}

inline vec rotate_y( vec v, float sin_ang, float cos_ang )
{
	return vec(
	    (v.x * cos_ang) + (v.z * sin_ang),
	    v.y,
	    (v.z * cos_ang) - (v.x * sin_ang)
	    );
}

viewport::viewport(int x, int y, int w, int h, const char *l)
    : Fl_Gl_Window(x, y, w, h, l) {
    }

void viewport::addModel(ObjModel& model) {
    models.push_back(model);
}

int viewport::handle(int event) {
    static int initialX = 0, initialY = 0;
    switch(event) {
        case FL_PUSH: {
            if(Fl::event_button2()) { // middle click
                arcball_start(Fl::event_x(), h() - Fl::event_y());
            }
            else {
                initialX = Fl::event_x();
                initialY = h() - Fl::event_y();
            }
            return 1;
        }
        case FL_DRAG: {
            if(Fl::event_button1()) { // left click
                deltaPosition.x = - 0.05 * (Fl::event_x() - initialX);
                deltaPosition.y = 0.05 * (h() - Fl::event_y() - initialY);
                initialX = Fl::event_x();
                initialY = h() - Fl::event_y();
            }
            else if(Fl::event_button2()) { // middle click
                eye.x += deltaPosition.x;
                arcball_move(Fl::event_x(), h() - Fl::event_y());
            }
            redraw();
            return 1;
        }
        break;
        case FL_MOUSEWHEEL: {
            deltaPosition.z -= 0.1 * Fl::event_dy();
            redraw();
            return 1;
        }
        break;
    }
}

void viewport::draw() {
    if(!valid()) {
        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClearDepth(1.0);
        glDepthFunc(GL_LESS);
        glEnable(GL_DEPTH_TEST);
        glShadeModel(GL_FLAT);

        static int width = w();
        static int height = h();
        static float aspect_ratio = (float) width / (float) height;

        glViewport(0, 0, width, height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective( 50.0f, aspect_ratio, 0.1f, 100.0f );

        gluLookAt(
                eye.x, eye.y, eye.z,
                centre.x, centre.y, centre.z,
                up.x, up.y, up.z );

        arcball_setzoom( SPHERE_RADIUS, eye, up );
    }


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity() ;

    /* For drawing the arcball sphere itself
     *
    glPushMatrix();
	glTranslatef( eye.x, eye.y, eye.z );
    glColor3f(1.0f,0.0f,0.0f);
    glutWireSphere(SPHERE_RADIUS,SPHERE_LONG_SLICES,SPHERE_LAT_SLICES);
    glPopMatrix();
    */

    world.x += deltaPosition.x;
    world.y += deltaPosition.y;
    world.z -= deltaPosition.z;

	glTranslatef( world.x, world.y, world.z + 2 );

    arcball_rotate();

    for(int i = 0; i < models.size(); i++) {
        models[i].draw(5 * i, 0, 0);
    }

    deltaPosition.x = 0;
    deltaPosition.y = 0;
    deltaPosition.z = 0;

}
