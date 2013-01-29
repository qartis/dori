#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Table.H>
#include "tableview.h"

int main(int argc, char **argv) {
    TableView *tableview = new TableView(0, 0, 400, 600, "test");

    tableview->end();
    tableview->show(argc, argv);
    return Fl::run();
}

