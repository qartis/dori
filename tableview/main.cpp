#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include "tableview.h"

int main(int argc, char **argv) {
    Fl_Window *window = new Fl_Double_Window(600,600);
    TableView *tableview = new TableView(0, 0, window->w(), window->h(), NULL);
    (void)tableview;
    window->end();
    window->show(argc, argv);
    return Fl::run();
}
