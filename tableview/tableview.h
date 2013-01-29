#include <FL/Fl.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Window.H>
#include <sqlite3.h>
#include "tableinput.h"
#include "table.h"
#include <vector>
#include "../radar/radarwindow.h"

#define BUFLEN 128

class TableView : public Fl_Window {

public:
    TableView(int x, int y, int w, int h, const char *label = NULL);

    virtual int handle(int event);
    void enableWidget(int row, int col, const char *label = NULL);

    Table* table;
    sqlite3 *db;
    sqlite3 *db_tmp;
    TableInput *queryInput;
    char buffer[BUFLEN];
    int bufMsgStartIndex;
    int bufReadIndex;
    int sockfd;

    void (*widgetDataCallback)(void*, void*);
    void (*widgetResetCallback)(void*);
    void *parentWidget;
    Fl_Window *widgetWindow;

    std::vector<SpawnableWindow*> spawned_windows;

private:
    static void performQuery(void *tableview, char *str);

};
