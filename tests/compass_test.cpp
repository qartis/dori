#include <FL/Fl.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Double_Window.H>
#include "fl_compass.h"

void handle_fd(int fd, void *data)
{
    Fl_Compass *c = (Fl_Compass *)data;
    double val;

    if (scanf("%lf", &val) != 1) {
        printf("fuck\n");
        exit(1);
    }

    c->value(val);
    c->redraw();
}

int main()
{
    Fl_Double_Window win(200,200,"Draw X");
    Fl_Compass compass(0, 0, win.w(), win.h());
    compass.value(180.0);
    win.resizable(compass);
    Fl::add_fd(0, handle_fd, &compass);
    win.show();
    return(Fl::run());
}
