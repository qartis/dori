#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Table.H>
#include <FL/Fl_Table_Row.H>
#include <FL/Fl_Input.H>
#include <stdlib.h>
#include <vector>
#include <sqlite3.h>
#include "../row.h"
#include "queryinput.h"
#include "ctype.h"
#include "table.h"
#include "mainwindow.h"

int main(int argc, char **argv) {
    MainWindow window(0, 0, 400, 600, "test");

    window.end();
    window.show(argc, argv);
    return Fl::run();
}

