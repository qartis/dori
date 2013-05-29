#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Table_Row.H>
#include <FL/Fl_Input.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>
#include <math.h>
#include <map>
#include <vector>
#include <limits.h>
#include "ctype.h"
#include "queryinput.h"
#include "row.h"
#include "graph.h"

Graph::Graph(int x, int y, int w, int h, const char *label) : Fl_Double_Window(x, y, w, h, label) {
}

void Graph::resize(int x, int y, int w, int h) {
    return Fl_Double_Window::resize(x, y, w, h);
}

void Graph::draw() {
}
