#ifndef __spawnablewindow_h_
#define __spawnablewindow_h_

#include <FL/Fl.H>
#include <FL/Fl_Window.H>

#include "mainwindow/row.h"

class SpawnableWindow : public Fl_Window {

public:
    SpawnableWindow(int x, int y, int w, int h, const char *label = NULL) : Fl_Window(x, y, w, h, label), rowData(NULL) { }

    std::vector<Row>* rowData;

};
#endif
