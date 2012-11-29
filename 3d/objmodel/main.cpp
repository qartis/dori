#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "viewport.h"
#include "objmodel.h"

int main(int argc, char **argv) {
    //Fl::gl_visual(FL_RGB);
    //Fl_Window *window = new Fl_Window(700,700);
    viewport *myviewport = new viewport(0, 0, 700, 700, NULL);
    ObjModel myModel;

    if(argc > 1)
    {
        myModel.load(argv[1]);
        myviewport->model = &myModel;
    }

    myviewport->end();
    myviewport->show();
    return Fl::run();
}
