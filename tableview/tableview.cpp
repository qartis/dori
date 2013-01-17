#include "tableview.h"
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

#define COL0_HEADER "type"
#define COL1_HEADER "a"
#define COL2_HEADER "b"
#define COL3_HEADER "c"
#define COL4_HEADER "timestamp"

#define PORT 1337
#define SERVER "127.0.0.1"

#define CELLWIDTH 115
#define CELLHEIGHT 25

int sqlite_cb(void *arg, int ncols, char **cols, char **rows)
{
    (void)rows;

    TableView *tview = (TableView*)arg;

    char buf[256];

    sprintf(buf, "%s,%s,%s,%s,%s", cols[0], cols[1], cols[2], cols[3], cols[4]);

    tview->table->add_row(buf);

    return 0;
}


static void performQuery(void *tableview, char *str) {
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
    int i;

    printf("rows to remove: %d\n", tview->table->rows());
    for(i = 0; i < tview->table->rows(); i++) {
        printf("removing row %d\n", i);
        tview->table->remove_row(i);
    }
    
    // set to 1 so we don't overwrite headers
    tview->totalRows = 1;
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
            //printf("inserting record %s %s %s %s %s %s\n", field[0], field[1], field[2], field[3], field[4], field[5]);
            sprintf(query, "INSERT INTO records (rowid, type, a, b, c, time) VALUES ('%s', '%s', '%s', '%s', '%s', '%s');", field[0], field[1], field[2], field[3], field[4], field[5]);
            sqlite3_exec(tview->db, query, NULL, NULL, NULL);
            //printf("insert: %s\n", sqlite3_errstr(sqlite3_errcode(tview->db)));
            index = 0;

            /*
            if(tview->queryInput->isLiveMode()) {
                sqlite3_exec(tview->db_tmp, query, NULL, NULL, NULL);
                sqlite3_exec(tview->db_tmp, tview->queryInput->getSearchString(), sqlite_cb, tview, NULL);
                sqlite3_exec(tview->db_tmp, "DELETE FROM records;", NULL, NULL, NULL);
                int limit = tview->queryInput->getLimit();
                if(limit >= 0) {
                    if(tview->totalRows > limit) {
                    }
                }
            }
            */
        }
    }
}


TableView::TableView(int x, int y, int w, int h, const char *label) : Fl_Window(x, y, w, h, label), totalRows(0), totalCols(0), db(NULL), queryInput(NULL), bufMsgStartIndex(0), bufReadIndex(0), sockfd(0), widgetDataCallback(NULL), parentWidget(NULL)
{
    queryInput = new TableInput(w * 0.2, 0, w * 0.75, CELLHEIGHT, "Search Query:");
    queryInput->callback = performQuery;

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
}

int TableView::handle(int event) {
    switch(event) {
    case FL_KEYDOWN:
    case FL_SHORTCUT: {
        int key = Fl::event_key();
        if(key == (FL_F + 1)) {
            if(!widgetWindow->shown()) {
                widgetWindow->show();
            }
            else {
                widgetWindow->hide();
            }
        }
    }
    default:
        return Fl_Window::handle(event);
    }
}

void TableView::enableWidget(int row, int col, const char *label) {
}

