#include <FL/Fl.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Double_Window.H>
#include "fl_gauge.h"

void handle_fd(int fd, void *data)
{
    Fl_Gauge *c = (Fl_Gauge *)data;
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
    Fl_Gauge gauge(0, 0, win.w(), win.h());
    gauge.value(50.0);
    gauge.angles(30, 360-30);
    gauge.range(0.0, 100.0);
    gauge.setScale(20, 2);
    win.resizable(gauge);
    Fl::add_fd(0, handle_fd, &gauge);
    win.show();
    return(Fl::run());
}
