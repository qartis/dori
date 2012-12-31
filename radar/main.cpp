#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include "radarwidget.h"

int main(int argc, char **argv) {
    Fl_Window window(600,600);
    RadarWidget radar(0, 0, window.w(), window.h(), NULL);
    window.end();
    window.show(argc, argv);
    return Fl::run();
}
