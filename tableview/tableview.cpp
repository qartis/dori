#include <FL/fl_draw.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Tile.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <errno.h>
#include <math.h>
#include <sqlite3.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "tableview.h"

#define COL0_HEADER "type"
#define COL1_HEADER "a"
#define COL2_HEADER "b"
#define COL3_HEADER "c"
#define COL4_HEADER "timestamp"

#define PORT 1337
#define SERVER "127.0.0.1"

int sqlite_cb(void *arg, int ncols, char **cols, char **rows)
{
    (void)rows;
    (void)ncols;

    char buf[256];

    sprintf(buf, "%s\t%s\t%s\t%s\t%s\t%s", cols[0], cols[1], cols[2], cols[3], cols[4], cols[5]);


    TableView *tview = (TableView*)arg;
    tview->table->add_row(buf);

    return 0;
}


void TableView::performQuery(void *tableview, char *str) {
    TableView *tview = (TableView*)tableview;
    tview->widgetResetCallback(tview);

    int err = sqlite3_exec(tview->db, str, sqlite_cb, tview, NULL);
    if(err != SQLITE_OK) {
        tview->queryInput->color(FL_RED);
        tview->queryInput->redraw();
    }
    else {
        tview->queryInput->color(FL_WHITE);
        tview->queryInput->redraw();
    }
}

static void clearTable(void *tableview) {
    TableView *tview = (TableView*)tableview;

    std::vector<SpawnableWindow*>::iterator it;
    for(it = tview->spawned_windows.begin(); it != tview->spawned_windows.end(); it++) {
        (*it)->redraw();
    }
    tview->table->clear();
}

static void handleFD(int fd, void *data) {
    TableView* tview = (TableView*)data;
    static char field[6][BUFLEN];
    char query[BUFLEN];
    static int index = 0;

    static char buf[BUFLEN];
    int rc;
    static int count = 0;

    rc = read(fd, buf + count, sizeof(buf) - count);
    if (rc <= 0) {
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

            if(rowid == -1) {
                tview->table->readyToDraw = 1;
                tview->queryInput->performQuery();
                index = 0;
                continue;
            }

            sprintf(query, "INSERT INTO records (rowid, type, a, b, c, time) VALUES ('%s', '%s', '%s', '%s', '%s', '%s');", field[0], field[1], field[2], field[3], field[4], field[5]);
            sqlite3_exec(tview->db, query, NULL, NULL, NULL);
            index = 0;

            if(tview->queryInput->isLiveMode()) {
                sqlite3_exec(tview->db_tmp, query, NULL, NULL, NULL);
                sqlite3_exec(tview->db_tmp, tview->queryInput->getSearchString(), sqlite_cb, tview, NULL);
                sqlite3_exec(tview->db_tmp, "DELETE FROM records;", NULL, NULL, NULL);
                tview->table->sort();
                int limit = tview->queryInput->getLimit();
                if(limit >= 0) {
                    if(tview->table->totalRows() > limit) {
                        int timestamp_col = 4;
                        int index = tview->table->minimum_row(timestamp_col);
                        tview->table->remove_row(index);
                    }
                }
            }
        }
    }
}

static void spawnRadarWindow(Fl_Widget *widget, void *data) {
    (void)widget;
    TableView *tview = (TableView*)data;

    RadarWindow *newRadar = new RadarWindow(0, 0, 600, 600);
    newRadar->rowData = &tview->table->_rowdata;
    newRadar->show();
    tview->spawned_windows.push_back(newRadar);
}

static void spawnArcballWindow(Fl_Widget *widget, void *data) {
    (void)widget;
    (void)data;
    //TableView *tview = (TableView*)data;
}

TableView::TableView(int x, int y, int w, int h, const char *label) : Fl_Window(x, y, w, h, label), db(NULL), queryInput(NULL), bufMsgStartIndex(0), bufReadIndex(0), sockfd(0), widgetDataCallback(NULL), parentWidget(NULL)
{
    queryInput = new TableInput(w * 0.2, 0, w * 0.75, 20, "Query:");
    queryInput->callback = performQuery;

    table = new Table(0, queryInput->h(), w, h - 20);
    table->selection_color(FL_YELLOW);
    table->col_header(1);
    table->col_resize(1);
    table->when(FL_WHEN_RELEASE);
    table->spawned_windows = &spawned_windows;

    add(queryInput);
    table->queryInput = queryInput;

    end();

    // resize this window to the size of the buttons below
    widgetWindow = new Fl_Window(w/2, h/2, 0, 0);

    Fl_Button *radar = new Fl_Button(5, 5, 80, 30, "Radar");
    Fl_Button *arcball = new Fl_Button(5, radar->y() + radar->h(), 80, 30, "Arcball");

    radar->callback(spawnRadarWindow, this);
    arcball->callback(spawnArcballWindow, this);
    widgetWindow->resize(0, 0, radar->w() + 10, radar->y() + radar->h() + arcball->h() + 5);
    widgetWindow->end();

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
        exit(0);
    }

    widgetResetCallback = clearTable;

    Fl::add_fd(sockfd, FL_READ, handleFD, (void*)this);

    sqlite3_open("", &db);
    sqlite3_exec(db, "CREATE TABLE records(type, a, b, c, time timestamp);", NULL, NULL, NULL);

    sqlite3_open("", &db_tmp);
    sqlite3_exec(db_tmp, "CREATE TABLE records(type, a, b, c, time timestamp);", NULL, NULL, NULL);

    queryInput->performQuery();
}

int TableView::handle(int event) {
    switch(event) {
    case FL_KEYDOWN: {
        int key = Fl::event_key();
        if(key == (FL_F + 1)) {
            if(!widgetWindow->shown()) {
                widgetWindow->show();
            }
            else {
                widgetWindow->hide();
            }
        }
        return 0;
    }
    default:
        return Fl_Window::handle(event);
    }

}
