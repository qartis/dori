#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Table.H>
#include <FL/Fl_Table_Row.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <stdlib.h>
#include <vector>
#include <sqlite3.h>
#include "row.h"
#include "queryinput.h"
#include "ctype.h"
#include "sparkline.h"
#include "table.h"
#include "querytemplatewidget.h"
#include "widgetwindow.h"
#include "mainwindow.h"

int main(int argc, char **argv) {
    MainWindow window(0, 0, 1024, 700, "Tableview");
    window.end();
    window.show(argc, argv);
    return Fl::run();
}

