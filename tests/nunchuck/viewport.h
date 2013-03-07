#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>
#include <GL/glu.h>

class viewport : public Fl_Gl_Window {

public:
    viewport(int x, int y, int w, int h, const char *l);

    void mapparams(GLfloat x, GLfloat y, GLfloat z);

private:
    float map(float input, float in_min, float in_max, float out_min, float out_max);
    void initcube();
    void draw();

    static GLfloat light_diffuse[];
    static GLfloat light_position[];
    static GLfloat n[6][3];
    static GLfloat v[8][3];
    static GLint faces[6][4];

    GLfloat xRot;
    GLfloat yRot;
    GLfloat zRot;

    GLfloat roll;
    GLfloat pitch;
};
