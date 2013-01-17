#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Table.H>
#include "tableview.h"

int main(int argc, char **argv) {
    TableView *tableview = new TableView(0, 0, 600, 600, NULL);
    (void)tableview;

    Table* table = new Table(0, 20, tableview->w(), tableview->h() - 20);
    tableview->table = table;


    tableview->end();
    tableview->show(argc, argv);
    return Fl::run();
}
