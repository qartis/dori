#include "viewport.h"
#include <math.h>

#define TOP    0,  0.5,  0
#define RIGHT  1, -1,  1
#define LEFT  -1, -1,  1
#define BACK   0, -1, -1

GLfloat viewport::n[6][3] = {  /* Normals for the 6 faces of a cube. */
    {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0},
    {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, -1.0} };
GLfloat viewport::v[8][3];
GLint viewport::faces[6][4] = {  /* Vertex indices for the 6 faces of a cube. */
    {0, 1, 2, 3}, {3, 2, 6, 7}, {7, 6, 5, 4},
    {4, 5, 1, 0}, {5, 6, 2, 1}, {7, 4, 0, 3} };

float viewport::map(float input, float in_min, float in_max, float out_min, float out_max) {
    return (input - in_min)*(out_max - out_min)/(in_max - in_min) + out_min;
}

viewport::viewport(int x, int y, int w, int h, const char *l)
    : xRot(0), yRot(0),
    Fl_Gl_Window(x, y, w, h, l) {}

void viewport::mapparams(GLfloat x, GLfloat y, GLfloat z) {
    // x = 284, 713,  (x - 287) * ((713.0-287.0)/360.0)
    xRot = x - 515;
    yRot = y - 515;
    zRot = z - 515;

    /*
        xRot = map(x, 284.0, 747.0, -M_PI, M_PI);
        yRot = map(y, 284.0, 747.0, -M_PI, M_PI);
        zRot = map(z, 284.0, 747.0, -M_PI, M_PI);
        */
    //yRot = (y - 291) * ((713.0-291.0)/360.0)/2.0;;

    roll = atan2(-xRot, sqrt(yRot*yRot + zRot*zRot)) * (180.0/M_PI);
    pitch =  atan2(-yRot, sqrt(xRot*xRot + zRot*zRot)) * (180.0/M_PI);

    printf("%f\t%f\t", roll, pitch);
    if (roll < -45.0) {
        roll = -90.0 + atan2(zRot, sqrt(yRot*yRot + xRot*xRot)) * (180.0/M_PI);
    } else if (roll > 45.0) {
        roll = 90.0 + atan2(-zRot, sqrt(yRot*yRot + xRot*xRot)) * (180.0/M_PI);
    }

    if (pitch < -45.0) {
        pitch = -90.0 + atan2(zRot, sqrt(xRot*xRot + yRot*yRot)) * (180.0/M_PI);
    } else if (pitch > 45.0) {
        pitch = 90.0 + atan2(-zRot, sqrt(xRot*xRot + yRot*yRot)) * (180.0/M_PI);
    }

    printf("%f\t%f\n", roll, pitch);
}

void viewport::initcube() {
    v[0][0] = v[1][0] = v[2][0] = v[3][0] = -1;
    v[4][0] = v[5][0] = v[6][0] = v[7][0] = 1;
    v[0][1] = v[1][1] = v[4][1] = v[5][1] = -1;
    v[2][1] = v[3][1] = v[6][1] = v[7][1] = 1;
    v[0][2] = v[3][2] = v[4][2] = v[7][2] = 1;
    v[1][2] = v[2][2] = v[5][2] = v[6][2] = -1;
}

void viewport::draw() {
    if (!valid()) {
        initcube();
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
    glTranslatef(0.0, 0.0, -5.0);
    // Position object
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(pitch, 1.0, 0.0, 0.0);
    glRotatef(roll, 0.0, 0.0, 1.0);

#if 1
        glColor3f(1.0, 0.0, 0.0); glBegin(GL_POLYGON); glVertex3f(TOP);   glVertex3f(RIGHT);  glVertex3f(LEFT);  glEnd();
        glColor3f(0.0, 1.0, 0.0); glBegin(GL_POLYGON); glVertex3f(TOP);   glVertex3f(BACK);   glVertex3f(RIGHT); glEnd();
        glColor3f(0.0, 0.0, 1.0); glBegin(GL_POLYGON); glVertex3f(TOP);   glVertex3f(LEFT);   glVertex3f(BACK);  glEnd();
        glColor3f(0.5, 0.5, 0.5); glBegin(GL_POLYGON); glVertex3f(RIGHT); glVertex3f(BACK);   glVertex3f(LEFT);  glEnd();
#else
    for (int i = 0; i < 6; i++) {
        glBegin(GL_QUADS);
        glColor3f(1/6.0*i, 0, 0);
        glNormal3fv(&n[i][0]);
        glVertex3fv(&v[faces[i][0]][0]);
        glVertex3fv(&v[faces[i][1]][0]);
        glVertex3fv(&v[faces[i][2]][0]);
        glVertex3fv(&v[faces[i][3]][0]);
        glEnd();
    }
#endif
}

