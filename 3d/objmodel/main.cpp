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


    /*
    fl_gl_contour *contour = new fl_gl_contour(172, 3, 593, 472, "no opengl");
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
    data.read_file("test.xyz",0,0);
    contour->set_input_data(data);
    contour->actualize();

    contour->set_number_x_grid(40);
    contour->set_number_y_grid(40);
    contour->set_number_z_grid(30);
    contour->graph_cb();
    */

    myviewport.Rmax=1.5;
    myviewport.resizable(myviewport);
    myviewport.show();

    return Fl::run();
}
