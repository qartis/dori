#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Table.H>
#include "mainwindow.h"

int main(int argc, char **argv) {
    MainWindow window(0, 0, 400, 600, "test");

    window.end();
    window.show(argc, argv);
    return Fl::run();
}

