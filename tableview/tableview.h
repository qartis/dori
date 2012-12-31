#include <FL/Fl.H>
#include <FL/Fl_Scroll.H>
#include <sqlite3.h>
#include "tableinput.h"

// hard limits on data size to keep things simple
#define MAXROWS 5200
#define MAXCOLS 4

#define BUFLEN 128

class TableView : public Fl_Scroll {

public:
    TableView(int x, int y, int w, int h, const char *label = NULL);

    virtual int handle(int event);
    void enableWidget(int row, int col, const char *label = NULL);

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

    void (*widgetDataCallback)(void*, void*);
    void (*widgetResetCallback)(void*);
    void *parentWidget;

private:

};
