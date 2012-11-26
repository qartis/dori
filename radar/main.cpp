#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include "radarwidget.h"

void readingTimer(void *data) {
    RadarWidget *radar = (RadarWidget*)data;
    int index;
    float distance;
    if (scanf("%d %f", &index, &distance) == 2) {
        radar->insertDataPoint(index, distance);
        radar->redraw();
        Fl::repeat_timeout(0.05, readingTimer, data);
    }
    else {
        printf("done reading\n");
    }
}


int main(int argc, char **argv) {
    Fl_Window *window = new Fl_Double_Window(600,600);
    RadarWidget *radar = new RadarWidget(0, 0, window->w(), window->h(), NULL);
    Fl::add_timeout(2.0, readingTimer, (void*)radar);
    window->end();
    window->show(argc, argv);
    return Fl::run();
}
