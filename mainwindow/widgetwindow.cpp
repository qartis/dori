#include <FL/fl_draw.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Table_Row.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Tile.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Output.H>
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
#include <fcntl.h>
#include "row.h"
#include "queryinput.h"
#include "ctype.h"
#include "table.h"
#include "objmodel.h"
#include "radarwindow.h"
#include "3dcontrols.h"
#include "basic_ball.h"
#include "Fl_Custom_Cursor.H"
#include "FlGlArcballWindow.h"
#include "mgl2/fltk.h"
#include "siteobject.h"
#include "lineobject.h"
#include "rectobject.h"
#include "circleobject.h"
#include "toolbar.h"
#include "siteeditor.h"
#include "viewport.h"
#include "widgetwindow.h"

static void spawnRadarWindow(Fl_Widget *widget, void *data) {
    (void)widget;
    WidgetWindow *window = (WidgetWindow*)data;

    if(window->table == NULL) {
        printf("table is null in spawnRadarWindow()\n");
    }

    RadarWindow *newRadar = new RadarWindow(0, 0, 600, 600);
    newRadar->user_data(&window->table->_rowdata);
    newRadar->show();
    window->table->spawned_windows->push_back(newRadar);
}

/*
static void graphInputCallback(Fl_Widget *widget, void *data) {
    WidgetWindow *window = (WidgetWindow*)data;
    char buf[256] = { 0 };
    sprintf(buf, "echo \"%s\" > plotfifo &", ((Fl_Input*)widget)->value());
    system(buf);
    write(window->controlFD, "plot \"plotfifo\"\n", strlen("plot \"plotfifo\"\n"));
}
*/

static void spawnModelViewer(Fl_Widget *widget, void *data) {
    (void)widget;
    WidgetWindow *window = (WidgetWindow*)data;

    if(window->table == NULL) {
        printf("table is null in spawnModelViewer()\n");
    }

    // showDORI = true
    Viewport *viewport = new Viewport(0, 0, 600, 600, NULL, false, true);
    viewport->user_data(&window->table->_rowdata);

    viewport->show();
    window->table->spawned_windows->push_back(viewport);
}

static void spawnSiteEditor(Fl_Widget *widget, void *data) {
    (void)widget;
    (void)data;
    //WidgetWindow *window = (WidgetWindow*)data;

    SiteEditor *siteEditor = new SiteEditor(0, 0, 600, 600, NULL);
    //siteEditor->user_data(&window->table->_rowdata);

    siteEditor->show();
    //window->table->spawned_windows->push_back(viewport);
}

static void graphCallback(Fl_Widget *widget, void *data) {
    (void)widget;
    WidgetWindow *window = (WidgetWindow*)data;

    system("rm plotfifo");
    system("mkfifo plotfifo");

    std::vector<Row>::iterator it = window->table->_rowdata.begin();

    system("gnuplot -p -e \"plot \\\"plotfifo\\\"\" &");

    int fd = open("plotfifo", O_WRONLY);

    int xAxis = window->xAxis->value();
    int yAxis = window->yAxis->value();

    for(; it != window->table->_rowdata.end(); it++) {
        write(fd, it->cols[xAxis], strlen(it->cols[xAxis]));
        write(fd, "\t", strlen("\t"));
        write(fd, it->cols[yAxis], strlen(it->cols[yAxis]));
        write(fd, "\n", strlen("\n"));
    }

    close(fd);

    /*
    system("mkfifo control");
    system("echo \"1 2\" > plotfifo &");
    popen("xterm -e \"gnuplot control\"", "w");
    window->controlFD = open("control", O_WRONLY);
    write(window->controlFD, "plot \"plotfifo\"\n", strlen("plot \"plotfifo\"\n"));
    window->graphInput->callback(graphInputCallback, data);
    window->graphInput->when(FL_WHEN_ENTER_KEY);
    */
}

WidgetWindow::WidgetWindow(int x, int y, int w, int h, const char *label, Table *t) : Fl_Window(x, y, w, h, label), table(t) {

    widgetGroupLabel = new Fl_Box(15, 5, 50, 20, "Widgets:");
    widgetGroupLabel->align(FL_ALIGN_CENTER);
    widgetGroupLabel->box(FL_NO_BOX);
    widgetGroupLabel->label();

    widgetGroup = new Fl_Window(10, 25, w - 20, 100);
    widgetGroup->box(FL_BORDER_BOX);
    widgetGroup->begin();
    radar = new Fl_Button(widgetGroup->x() + 5, 5, 150, 30, "Radar");
    modelViewer = new Fl_Button(radar->x(), radar->y() + radar->h(), 150, 30, "3D");
    siteEditor = new Fl_Button(modelViewer->x(), modelViewer->y() + modelViewer->h(), 150, 30, "Site Editor");
    widgetGroup->end();

    graphGroupLabel = new Fl_Box(15, widgetGroup->y() + widgetGroup->h() + 15, 60, 20, "Graphing:");
    graphGroupLabel->align(FL_ALIGN_CENTER);
    graphGroupLabel->box(FL_NO_BOX);
    graphGroupLabel->label();

    graphGroup = new Fl_Window(10, graphGroupLabel->y() + graphGroupLabel->h(), w - 20, 140);
    graphGroup->box(FL_BORDER_BOX);
    graphGroup->begin();
    xAxis = new Fl_Choice(graphGroup->x() + 60, 5, 100, 30, "X Axis");

    yAxis = new Fl_Choice(xAxis->x(), xAxis->y() + xAxis->h(), 100, 30, "Y Axis");
    graph = new Fl_Button(graphGroup->x(), yAxis->y() + yAxis->h() + 4, 160, 30, "Graph");
    /*
    graphInput = new Fl_Input(graphGroup->x(), graph->y() + graph->h() + 20, 160, 20, "Graph Input");
    graphInput->align(FL_ALIGN_TOP);
    */
    graphGroup->end();

    radar->callback(spawnRadarWindow, this);
    modelViewer->callback(spawnModelViewer, this);
    siteEditor->callback(spawnSiteEditor, this);
    graph->callback(graphCallback, this);

    if(table) {
        std::vector<const char*>::iterator it = table->headers->begin();
        for(; it != table->headers->end(); it++) {
            xAxis->add(*it);
            yAxis->add(*it);
        }
    }

    xAxis->value(0);
    yAxis->value(0);

    end();
}

int WidgetWindow::handle(int event) {
    switch(event) {
    case FL_KEYDOWN: {
        int key = Fl::event_key();
        if(key == (FL_F + 1)) {
            if(!shown()) {
                show();
            }
            else {
                hide();
            }
        }
        return 1;
    }
    default:
        return Fl_Window::handle(event);
    }
}
