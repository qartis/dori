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
#include <fcntl.h>
#include "row.h"
#include "queryinput.h"
#include "ctype.h"
#include "sparkline.h"
#include "table.h"
#include "objmodel.h"
#include "radarwindow.h"
#include "3dcontrols.h"
#include "basic_ball.h"
#include "Fl_Custom_Cursor.H"
#include "FlGlArcballWindow.h"
#include "siteobject.h"
#include "rectobject.h"
#include "colorchooser.h"
#include "toolbar.h"
#include "siteeditor.h"
#include "siteeditorwindow.h"
#include "viewport.h"
#include "widgetwindow.h"
#include "mainwindow.h"

static void window_cb(Fl_Widget *widget, void *data) {
    WidgetWindow* widgetwindow = (WidgetWindow*)widget;
    if(widget->user_data()) {
        MainWindow* mw = (MainWindow*)widget->user_data();
        mw->hide();
    }
    widgetwindow->hide();
}

static void spawnRadarWindow(Fl_Widget *widget, void *data) {
    (void)widget;
    WidgetWindow *window = (WidgetWindow*)data;

    if(window->table == NULL) {
        printf("table is null in spawnRadarWindow()\n");
    }

    RadarWindow *newRadar = new RadarWindow(0, 0, 600, 600);
    newRadar->table = window->table;
    newRadar->show();
    window->table->spawned_windows->push_back(newRadar);
}

static void spawnModelViewer(Fl_Widget *widget, void *data) {
    (void)widget;
    WidgetWindow *window = (WidgetWindow*)data;

    if(window->table == NULL) {
        printf("table is null in spawnModelViewer()\n");
    }

    // showDORI = true
    Viewport *viewport = new Viewport(0, 0, 600, 600, NULL, true, true);
    viewport->table = window->table;

    viewport->show();
    window->table->spawned_windows->push_back(viewport);
}

static void spawnSiteEditor(Fl_Widget *widget, void *data) {
    (void)widget;
    (void)data;
    //WidgetWindow *window = (WidgetWindow*)data;

    SiteEditorWindow *siteEditor = new SiteEditorWindow(0, 0, 700, 700, NULL);
    //siteEditor->user_data(&window->table->rowdata);

    siteEditor->show();
    //window->table->spawned_windows->push_back(viewport);
}

static void graphCallback(Fl_Widget *widget, void *data) {
    (void)widget;
    WidgetWindow *window = (WidgetWindow*)data;

    system("rm plotfifo");
    system("mkfifo plotfifo");

    std::vector<Row>::iterator it = window->table->rowdata.begin();

    system("gnuplot -p -e \"plot \\\"plotfifo\\\" with lines\" &");

    int fd = open("plotfifo", O_WRONLY);

    int xAxis = window->xAxis->value();
    int yAxis = window->yAxis->value();

    for(; it != window->table->rowdata.end(); it++) {
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

static void shellCallback(Fl_Widget *widget, void *data) {
    (void)widget;
    (void)data;
    popen("xterm -e \"PATH=`pwd`:$PATH;tail -f shell.log & bash\"", "w");
}



WidgetWindow::WidgetWindow(int x, int y, int w, int h, const char *label, Table *t) : Fl_Window(x, y, w, h, label), table(t) {

    widgetGroupLabel = new Fl_Box(15, 5, 50, 20, "Widgets:");
    widgetGroupLabel->align(FL_ALIGN_CENTER);
    widgetGroupLabel->box(FL_NO_BOX);
    widgetGroupLabel->label();

    widgetGroup = new Fl_Window(10, 25, w - 20, 140);
    widgetGroup->box(FL_BORDER_BOX);
    widgetGroup->begin();
    radar = new Fl_Button(widgetGroup->x() + 5, 5, 150, 30, "LIDAR");
    modelViewer = new Fl_Button(radar->x(), radar->y() + radar->h(), 150, 30, "3D visualizer");
    siteEditor = new Fl_Button(modelViewer->x(), modelViewer->y() + modelViewer->h(), 150, 30, "Site editor");

    shell = new Fl_Button(siteEditor->x(), siteEditor->y() + siteEditor->h(), 150, 30, "Shell");

    widgetGroup->end();

    graphGroupLabel = new Fl_Box(15, widgetGroup->y() + widgetGroup->h() + 15, 60, 20, "Graphing:");
    graphGroupLabel->align(FL_ALIGN_CENTER);
    graphGroupLabel->box(FL_NO_BOX);
    graphGroupLabel->label();

    graphGroup = new Fl_Window(10, graphGroupLabel->y() + graphGroupLabel->h(), w - 20, 120);
    graphGroup->box(FL_BORDER_BOX);
    graphGroup->begin();
    xAxis = new Fl_Choice(graphGroup->x() + 60, 5, 100, 30, "X Axis");

    yAxis = new Fl_Choice(xAxis->x(), xAxis->y() + xAxis->h(), 100, 30, "Y Axis");
    graph = new Fl_Button(graphGroup->x(), yAxis->y() + yAxis->h() + 4, 160, 30, "2D Visualizer");
    /*
    graphInput = new Fl_Input(graphGroup->x(), graph->y() + graph->h() + 20, 160, 20, "Graph Input");
    graphInput->align(FL_ALIGN_TOP);
    */
    graphGroup->end();

    radar->callback(spawnRadarWindow, this);
    modelViewer->callback(spawnModelViewer, this);
    siteEditor->callback(spawnSiteEditor, this);
    graph->callback(graphCallback, this);
    shell->callback(shellCallback, this);

    callback(window_cb);

    end();
}

int WidgetWindow::handle(int event) {
    switch(event) {
    case FL_KEYDOWN: {
        int key = Fl::event_key();
        if(key == (FL_F + 1)) {
            if(user_data()) {
                MainWindow* mw = (MainWindow*)user_data();
                if(!mw->shown()) {
                    mw->show();
                }
                else {
                    mw->hide();
                }
            }
        }
        return 1;
    }

    default:
        return Fl_Window::handle(event);
    }
}
