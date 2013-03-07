#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

GLfloat light_diffuse[] = {1.0, 0.0, 0.0, 1.0};  /* Red diffuse light. */
GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};  /* Infinite light location. */
GLfloat n[6][3] = {  /* Normals for the 6 faces of a cube. */
  {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0},
  {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, -1.0} };
GLint faces[6][4] = {  /* Vertex indices for the 6 faces of a cube. */
  {0, 1, 2, 3}, {3, 2, 6, 7}, {7, 6, 5, 4},
  {4, 5, 1, 0}, {5, 6, 2, 1}, {7, 4, 0, 3} };
GLfloat v[8][3];  /* Will be filled in with X,Y,Z vertexes. */

void drawBox(void){
    int i;

    for (i = 0; i < 6; i++) {
        glBegin(GL_QUADS);
        glNormal3fv(&n[i][0]);
        glVertex3fv(&v[faces[i][0]][0]);
        glVertex3fv(&v[faces[i][1]][0]);
        glVertex3fv(&v[faces[i][2]][0]);
        glVertex3fv(&v[faces[i][3]][0]);
        glEnd();
    }
}

void display(void){
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawBox();
  glutSwapBuffers();
}

void init(void){
  /* Setup cube vertex data. */
  v[0][0] = v[1][0] = v[2][0] = v[3][0] = -1;
  v[4][0] = v[5][0] = v[6][0] = v[7][0] = 1;
  v[0][1] = v[1][1] = v[4][1] = v[5][1] = -1;
  v[2][1] = v[3][1] = v[6][1] = v[7][1] = 1;
  v[0][2] = v[3][2] = v[4][2] = v[7][2] = 1;
  v[1][2] = v[2][2] = v[5][2] = v[6][2] = -1;

  /* Enable a single OpenGL light. */
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);

  /* Use depth buffering for hidden surface elimination. */
  glEnable(GL_DEPTH_TEST);

  /* Setup the view of the cube. */
  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 40.0,
    /* aspect ratio */ 1.0,
    /* Z near */ 1.0, /* Z far */ 10.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,5) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in positive Y direction */

  /* Adjust cube position to be asthetic angle. */
  glTranslatef(0.0, 0.0, -1.0);
  glRotatef(60, 1.0, 0.0, 0.0);
  glRotatef(-20, 0.0, 0.0, 1.0);
}

float x,y,z;

double map(double x, double in_min, double in_max, double out_min, double out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void timer(void){
    char buf[128];
    static int prev_x=0, prev_y=0, prev_z=0;
    int xx,yy,zz;
    int foo;
    //287 713 275 792 274 738
    if (scanf("%d %d %d %d %d",&foo, &foo, &xx,&yy,&zz) != 5) {
        printf("error reading\n");
        exit(1);
    }

    x = map(x, 270, 713, -M_PI, M_PI);
    y = map(y, 270, 792, -M_PI, M_PI);
    z = map(z, 270, 738, -M_PI, M_PI);

    double roll = atan2(y, z) * 180/M_PI;
    double pitch = atan2(x, sqrt(y*y + z*z)) * 180/M_PI;

    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,5) */
            0.0, 0.0, 0.0,      /* center is at (0,0,0) */
            0.0, 1.0, 0.);      /* up is in positive Y direction */

    /* Adjust cube position to be asthetic angle. */
    glTranslatef(0.0, 0.0, -1.0);
    //glRotatef(60, 1.0, 0.0, 0.0);
    //glRotatef(-20, 0.0, 0.0, 1.0);
    glRotatef(pitch,0.0,0.0,1.0);
    glRotatef(roll,1.0,0.0,0.0);

    display();
}


int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutCreateWindow("nunchuck 3d cube");
    glutDisplayFunc(display);
    init();
    glutIdleFunc(timer);
    glutMainLoop();
    return 0;
}
