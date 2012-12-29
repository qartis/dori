#include <FL/Fl.H>
#include <FL/Fl_Scroll.H>
#include <sqlite3.h>
#include "tableinput.h"

// hard limits on data size to keep things simple
#define MAXROWS 5200
#define MAXCOLS 6

#define CELLWIDTH 200
#define CELLHEIGHT 25

#define BUFLEN 128

class TableView : public Fl_Scroll {

public:
    TableView(int x, int y, int w, int h, const char *label);

    virtual int handle(int event);
    //virtual void draw();
    Fl_Widget* widgets[MAXROWS][MAXCOLS];
    int totalRows;
    int totalCols;
    sqlite3 *db;
    TableInput *queryInput;
    char buffer[BUFLEN];
    int bufMsgStartIndex;
    int bufReadIndex;
    int sockfd;

    bool liveMode;

    void enableWidget(int row, int col, const char *label);

private:

};
