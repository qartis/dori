#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "viewport.h"
#include "objmodel.h"
#include <GL/glut.h>

int main(int argc, char **argv) {
    //Fl::gl_visual(FL_RGB);
    //Fl_Window *window = new Fl_Window(700,700);
    viewport *myviewport = new viewport(0, 0, 700, 700, NULL);
    ObjModel model1;
    ObjModel model2;

    glutInit(&argc, argv);

    if(argc > 1)
    {
        model1.load(argv[1]);
        myviewport->addModel(model1);

        if(argc > 2) {
            model2.load(argv[2]);
            myviewport->addModel(model2);
        }
    }

    myviewport->end();
    myviewport->show();
    return Fl::run();
}
