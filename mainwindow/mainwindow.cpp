#include <FL/fl_draw.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Table_Row.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Tile.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Color_Chooser.H>
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
#include "ctype.h"
#include "table.h"
#include "objmodel.h"
#include "radarwindow.h"
#include "3dcontrols.h"
#include "basic_ball.h"
#include "FlGlArcballWindow.h"
#include "siteobject.h"
#include "lineobject.h"
#include "rectobject.h"
#include "circleobject.h"
#include "toolbar.h"
#include "viewport.h"
#include "widgetwindow.h"
#include "mainwindow.h"
#include <sys/time.h>

#define PORT 1337
#define SERVER "127.0.0.1"

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

    MainWindow *window = (MainWindow*)arg;
    int index = 0;

    for(int i = 0; i < ncols; i++) {
        if(cols[i] == NULL) {
            sprintf(&buf[index++], "\t");
            continue;
        }
        sprintf(&buf[index], "%s\t", cols[i]);
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

    window->table->add_row(buf, window->greenify);

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
    window->greenify = false;

    /*
    timeval stop, start;
    printf("before sqlite3_exec\n");
    gettimeofday(&start, NULL);
    */

    int err = sqlite3_exec(window->db, window->queryInput->getSearchString(), sqlite_cb, window, NULL);
    if(err != SQLITE_OK) {
        window->queryInput->color(FL_RED);
    }
    else {
        /*gettimeofday(&stop, NULL);
        printf("after sqlite3_exec\n");
        printf("took %lu\n", stop.tv_usec - start.tv_usec);
        */
        window->queryInput->color(FL_WHITE);
        if(window->needFlush) {
            window->clearTable(window);
        }
    }

    window->table->autowidth(20);
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

static void handleFD(int fd, void *data) {
    MainWindow* window = (MainWindow*)data;
    static char field[6][BUFLEN];
    char query[BUFLEN];
    static int index = 0;

    static char buf[BUFLEN];
    int rc;
    static int count = 0;

    rc = read(fd, buf + count, sizeof(buf) - count);

    if (rc <= 0) {
        Fl::remove_fd(fd);
        return;
    }

    count += rc;

    char * pos;

    while((pos = (char *)memchr(buf, '\0', count)) != NULL) {
        memcpy(field[index], buf, pos - buf + 1);
        count -= (pos - buf + 1);
        memmove(buf, pos + 1, count);
        index++;

        if(index == 6) {
            int rowid = atoi(field[0]);

            if(window->table->readyToDraw) {
                window->greenify = true;
            }

            if(rowid == -1) {
                int err = sqlite3_exec(window->db, "END TRANSACTION", NULL, NULL, NULL);
                if(err != SQLITE_OK) {
                    printf("sqlite error: %s\n", sqlite3_errmsg(window->db));
                }

                window->shell_log = fopen(window->shell_log_filename, "a");
                setbuf(window->shell_log, NULL);

                window->table->readyToDraw = 1;
                window->receivedFirstDump = true;
                window->performQuery(window);
                index = 0;
                continue;
            }


            if(window->shell_log) {
                time_t ltime;
                ltime=time(NULL);
                char timestamp[128];
                int length = sprintf(timestamp, "%s", asctime(localtime(&ltime)));
                timestamp[length-1] = '\0';

                fprintf(window->shell_log, TERM_RED "%s: '%s' '%s' '%s' '%s' '%s'\n" TERM_NORM, timestamp, field[1], field[2], field[3], field[4], field[5]);
            }

            sprintf(query, "INSERT INTO records (rowid, type, a, b, c, time) VALUES (%s, '%s', %s, %s, %s, %s);", field[0], field[1], field[2], field[3], field[4], field[5]);
            sqlite3_exec(window->db, query, NULL, NULL, NULL);
            index = 0;


            if(window->receivedFirstDump) {
                sqlite3_exec(window->db_tmp, query, NULL, NULL, NULL);
                sqlite3_exec(window->db_tmp, window->queryInput->getSearchString(), window->sqlite_cb, window, NULL);

                sqlite3_exec(window->db_tmp, "DELETE FROM records;", NULL, NULL, NULL);
                window->table->sortUI();
                int limit = window->queryInput->getLimit();
                if(limit > 0) {
                    if(window->table->totalRows() > limit) {
                        window->table->remove_last_row();
                    }
                }
            }

            window->greenify = false;
        }
    }
}


MainWindow::MainWindow(int x, int y, int w, int h, const char *label) : Fl_Window(x, y, w, h, label), db(NULL), db_tmp(NULL), queryInput(NULL), bufMsgStartIndex(0), bufReadIndex(0), sockfd(0), needFlush(false), greenify(false), receivedFirstDump(false), shell_log(NULL)
{
    sqlite3_enable_shared_cache(1);
    queryInput = new QueryInput(w * 0.2, 0, w * 0.75, 20, "Query:");
    queryInput->performQuery = performQuery;
    queryInput->testQuery = testQuery;

    table = new Table(0, queryInput->h(), w, h - 20);
    table->selection_color(FL_YELLOW);
    table->col_header(1);
    table->col_resize(1);
    table->when(FL_WHEN_RELEASE);
    table->spawned_windows = &spawned_windows;
    table->headers = &headers;

    headers.push_back("rowid");
    headers.push_back("type");
    headers.push_back("a");
    headers.push_back("b");
    headers.push_back("c");
    headers.push_back("time");

    add(queryInput);
    table->queryInput = queryInput;

    end();

    // resize this window to the size of the buttons below
    widgetWindow = new WidgetWindow(w, 0, 200, 340, NULL, table);
    widgetWindow->user_data(this);
    widgetWindow->show();

    struct sockaddr_in serv_addr;
    struct hostent *server = NULL;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        printf("error opening socket\n");
        exit(0);
    }

    server = gethostbyname(SERVER);
    if(server == NULL) {
        printf("error, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(PORT);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("error connecting to server\n");
    }
    else {
        Fl::add_fd(sockfd, FL_READ, handleFD, (void*)this);

        sqlite3_open("", &db);
        sqlite3_exec(db, "CREATE TABLE records(type, a, b, c, time timestamp);", NULL, NULL, NULL);

        sqlite3_open("", &db_tmp);
        sqlite3_exec(db_tmp, "CREATE TABLE records(type, a, b, c, time timestamp);", NULL, NULL, NULL);

        int err = sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
        if(err != SQLITE_OK) {
            printf("sqlite error: %s\n", sqlite3_errmsg(db));
        }
    }
}

int MainWindow::handle(int event) {
    switch(event) {
    case FL_KEYDOWN: {
        int key = Fl::event_key();
        if(key == (FL_F + 1)) {
            hide();
        }
        if(key == (FL_F + 5)) {
            table->clearNewQueries();
        }

        return 1;
    }
    default:
        return Fl_Window::handle(event);
    }
}
