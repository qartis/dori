#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "viewport.h"

void Timer_CB(int unused, void *data) {
    viewport *viewportPtr = (viewport*)data;
    int xx,yy,zz;
    int foo;
    //287 713 275 792 274 738
    if (scanf("%d %d %d %d %d",&foo, &foo, &xx,&yy, &zz) == 5) {
        viewportPtr->mapparams(xx, yy, zz);
        viewportPtr->redraw();
    }
    else {
        printf("fucked\n");
        exit(1);
    }
}

int main(int argc, char **argv) {
    //Fl::gl_visual(FL_RGB);
    //Fl_Window *window = new Fl_Window(700,700);
    viewport *myviewport = new viewport(0, 0, 700, 700, NULL);

    Fl::add_fd(0, Timer_CB, (void*)myviewport);
    myviewport->end();
    myviewport->show(argc, argv);
    return Fl::run();
}
