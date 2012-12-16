#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include "viewport.h"
#include "objmodel.h"

int main(int argc, char **argv) {
    //Fl::gl_visual(FL_RGB);

    //Fl_Window *window = new Fl_Double_Window(700,700);

    viewport myviewport(640, 480, "test");
    ObjModel model1;
    ObjModel model2;

    if(argc > 1)
    {
        model1.load(argv[1]);
        myviewport.addModel(model1);

        if(argc > 2) {
            model2.load(argv[2]);
            myviewport.addModel(model2);
        }
    }

    myviewport.Rmax=1.5;
    myviewport.resizable(myviewport);
    myviewport.show();

    return Fl::run();
}
