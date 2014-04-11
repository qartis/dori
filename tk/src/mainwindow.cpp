#include <FL/fl_draw.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Table_Row.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl_Tree.H>
#include <errno.h>
#include <math.h>
#include <sqlite3.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <list>
#include <string.h>
#include <map>
#include "row.h"
#include "queryinput.h"
#include "basic_ball.h"
#include "FlGlArcballWindow.h"
#include "ctype.h"
#include "sparkline.h"
#include "table.h"
#include "objmodel.h"
#include "radarwindow.h"
#include "siteobject.h"
#include "lineobject.h"
#include "rectobject.h"
#include "circleobject.h"
#include "colorchooser.h"
#include "toolbar.h"
#include "viewport.h"
#include "querytemplatewidget.h"
#include "widgetwindow.h"
#include "mainwindow.h"
#include <time.h>

// modified version of Fl_File_Chooser
// exposed fileName (text edit)
#include "file_chooser.h"

#define TERM_NORM  "\x1B[0m"
#define TERM_RED  "\x1B[31m"
#define TERM_GREEN  "\x1B[32m"
#define TERM_YELLOW  "\x1B[33m"
#define TERM_BLUE  "\x1B[34m"
#define TEMR_MAGENTA  "\x1B[35m"
#define TERM_CYAN  "\x1B[36m"
#define TERM_WHITE  "\x1B[37m"

const char* MainWindow::shell_log_filename = "shell.log";

int MainWindow::sqlite_cb(void *arg, int ncols, char **cols, char **colNames)
{
    char buf[512] = { 0 };
    int index = 0;

    MainWindow *window = (MainWindow*)arg;

    Row newRow;

    for (int i = 0; i < ncols; i++) {
        if(cols[i] == NULL) {
            sprintf(&buf[index++], "\t");
            newRow.cols.push_back(strdup("\t"));
            continue;
        }
        sprintf(&buf[index], "%s", cols[i]);
        //printf("cb %s\n", cols[i]);
        newRow.cols.push_back(strdup(&buf[index]));
        index += strlen(cols[i]) + 1;
    }

    if(window->needFlush) {
        window->headers.clear();

        for(int i = 0; i < ncols; i++) {
            window->headers.push_back(strdup(colNames[i]));
        }

        window->widgetWindow->xAxis->clear();
        window->widgetWindow->yAxis->clear();

        std::vector<const char*>::iterator it = window->table->headers->begin();
        for(; it != window->table->headers->end(); it++) {
            window->widgetWindow->xAxis->add(*it);
            window->widgetWindow->yAxis->add(*it);
        }

        window->widgetWindow->xAxis->value(0);
        window->widgetWindow->yAxis->value(0);

        window->clearTable(window);
        window->needFlush = false;
    }

    window->table->add_row(newRow);

    return 0;
}

void MainWindow::testQuery(void *arg) {
    MainWindow *window = (MainWindow*)arg;

    int err = sqlite3_exec(window->db, window->queryInput->getSearchString(), NULL, window, NULL);
    if(err != SQLITE_OK) {
        window->queryInput->color(FL_RED);
    }
    else {
        window->queryInput->color(FL_WHITE);
    }

    window->queryInput->redraw();
}

void MainWindow::performQuery(void *arg) {
    MainWindow *window = (MainWindow*)arg;

    window->needFlush = true;

    window->table->readyToDraw = 0;
    int err = sqlite3_exec(window->db, window->queryInput->getSearchString(), sqlite_cb, window, NULL);
    window->table->readyToDraw = 1;
    if(err != SQLITE_OK) {
        window->queryInput->color(FL_RED);
    }
    else {
        window->queryInput->color(FL_WHITE);
        if(window->needFlush) {
            window->clearTable(window);
        }
    }

    window->table->autowidth(0);
    window->table->done();
    window->queryInput->redraw();
    window->needFlush = false;
}

void MainWindow::clearTable(void *arg) {
    MainWindow *window = (MainWindow*)arg;
    window->table->clear();

    std::vector<Fl_Window*>::iterator it;
    for(it = window->spawned_windows.begin(); it != window->spawned_windows.end(); it++) {
        (*it)->redraw();
    }
}

MainWindow::MainWindow(int x, int y, int w, int h, const char *label) : Fl_Window(x, y, w, h, label), db(NULL), queryInput(NULL), bufMsgStartIndex(0), bufReadIndex(0), needFlush(false), receivedFirstDump(false), shell_log(NULL)
{
    queryInput = new QueryInput(w * 0.2, 0, w * 0.75, 20, "Query:");
    queryInput->performQuery = performQuery;
    queryInput->testQuery = testQuery;

    table = new Table(0, queryInput->h(), w, h - 20);
    table->selection_color(FL_YELLOW);
    table->when(FL_WHEN_RELEASE);
    table->spawned_windows = &spawned_windows;
    table->headers = &headers;
    table->col_header(1);
    table->col_resize(1);

    add(queryInput);
    table->queryInput = queryInput;

    sqlite3_open("/tmp/db", &db);

    queryTemplateWidget = new QueryTemplateWidget(db, table->w(), 0, 300, table->h());

    end();

    // resize this window to the size of the buttons below
    widgetWindow = new WidgetWindow(w, 0, 200, 340, NULL, table);
    widgetWindow->user_data(this);
    widgetWindow->show();
}

int MainWindow::handle(int event) {
    switch(event) {
    case FL_KEYDOWN:
    case FL_KEYUP: {
        int key = Fl::event_key();
        printf("keydown %d\n", key);
        if(key == (FL_F + 1)) {
            hide();
            return 1;
        }
        else if(key == 's' && Fl::event_ctrl()) {
            printf("chooser\n");
            Fl_File_Chooser chooser(".",
                                    "*.csv",
                                    Fl_File_Chooser::CREATE,
                                    "Save dataset as CSV...");
            chooser.fileName->when(FL_WHEN_CHANGED | FL_WHEN_ENTER_KEY_ALWAYS);
            chooser.show();

            while(chooser.shown()) {
                Fl::wait();
            }

            if (chooser.value() != NULL) {
                printf("saving to %s\n", chooser.value());

                std::vector<Row>::iterator it = table->rowdata.begin();

                FILE *fp = fopen(chooser.value(), "w");

                if(fp == NULL) {
                    perror("fopen()");
                    return 0;
                }
                for(; it != table->rowdata.end(); it++) {
                    for(unsigned i = 0; i < it->cols.size(); i++) {
                        fprintf(fp, "%s", it->cols[i]);

                        if(i < it->cols.size() - 1) {
                            fprintf(fp, ",");
                        }
                    }

                    fprintf(fp, "\n");
                }

                fclose(fp);
            }
            return 1;
        }
    }
    default:
        return Fl_Window::handle(event);
    }
}
