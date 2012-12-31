#include "tableview.h"
#include <FL/fl_draw.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Box.H>
#include <errno.h>
#include <math.h>
#include <sqlite3.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define COL0_HEADER "type"
#define COL1_HEADER "val1"
#define COL2_HEADER "val2"
#define COL3_HEADER "timestamp"

#define PORT 1337
#define SERVER "localhost"

#define CELLWIDTH 150
#define CELLHEIGHT 25

static void performQuery(void *tableview, void *str) {
    TableView *tview = (TableView*)tableview;
    char *query = (char *)str;
    int nbytes = write(tview->sockfd, query, strlen(query));
    if(nbytes < 0) {
        printf("error writing to socket: %d\n", errno);
        exit(0);
    }
}

static void handleFD(int fd, void *data) {
    TableView* tview = (TableView*)data;
    char *buffer = tview->buffer;
    char c;
    char type[BUFLEN];
    char col1[BUFLEN];
    char col2[BUFLEN];
    char time[BUFLEN];
    int rc;

    rc = read(fd, &c, 1);
    if (rc <= 0) {
        return;
    }

    tview->buffer[tview->bufReadIndex] = c;

    static int clear = 0;

    switch (c) {
    case '!':
        tview->queryInput->color(FL_RED);
        tview->redraw();
        break;
    case '^':
        clear = 1;
        break;

    case '@':
        tview->redraw();
    case '{':
        if (clear == 1) {
            tview->queryInput->color(FL_WHITE);
            if (!tview->liveMode) {
                for (int i = 1; i < tview->totalRows; i++) {
                    for (int j = 0; j < tview->totalCols; j++) {
                        tview->remove(tview->widgets[i][j]);
                    }
                }
                tview->totalRows = 1;
                tview->bufReadIndex = 0;
                tview->bufMsgStartIndex = 0;
                tview->bufReadIndex = 0;
            }
            tview->redraw();

            if(tview->widgetResetCallback != NULL) {
                tview->widgetResetCallback(tview->parentWidget);
            }


            clear = 0;
        }
        tview->bufMsgStartIndex = tview->bufReadIndex+1;
        break;

    case '}':
        tview->buffer[tview->bufReadIndex] = '\0';

        // parse data
        // reset colour
        //tview->queryInput->color(FL_WHITE);

        buffer = &tview->buffer[tview->bufMsgStartIndex];

        // pass the new data to our widget (radar, 3d, etc)
        if(tview->widgetDataCallback != NULL) {
            tview->widgetDataCallback(buffer, tview->parentWidget);
        }

        // most of the stuff below is hardcoded and temporary
        sscanf(buffer, "%[^,],%[^,],%[^,],%[^,]",type, col1, col2, time);

        tview->enableWidget(tview->totalRows, 0, type);
        tview->enableWidget(tview->totalRows, 1, col1);
        tview->enableWidget(tview->totalRows, 2, col2);
        tview->enableWidget(tview->totalRows, 3, time);

        tview->totalRows++;

        tview->bufMsgStartIndex = 0;
        tview->bufReadIndex = 0;
        break;

    case '\0':
        return;
    }

    tview->bufReadIndex++;
}

TableView::TableView(int x, int y, int w, int h, const char *label) : Fl_Scroll(x, y, w, h, label), totalRows(0), totalCols(0), db(NULL), queryInput(NULL), bufMsgStartIndex(0), bufReadIndex(0), sockfd(0), liveMode(false), widgetDataCallback(NULL), parentWidget(NULL)
{
    queryInput = new TableInput(w * 0.2, 0, w * 0.75, CELLHEIGHT, "Search Query:");
    queryInput->callback = performQuery;

    end(); // don't add the cell widgets to the table view automatically

    for(int i = 0; i < MAXROWS; i++) {
        for(int j = 0; j < MAXCOLS; j++) {
            widgets[i][j] = new Fl_Box(CELLWIDTH * j, queryInput->y() + queryInput->h() + (CELLHEIGHT * i), CELLWIDTH, CELLHEIGHT);
        }
    }

    // enable the column header widgets
    enableWidget(0, 0, COL0_HEADER);
    enableWidget(0, 1, COL1_HEADER);
    enableWidget(0, 2, COL2_HEADER);
    enableWidget(0, 3, COL3_HEADER);

    totalCols = MAXCOLS;

    /*
       sqliteResult = sqlite3_open("dori.db", &db);
       if(sqliteResult) {
       printf("sqlite error: %s\n", sqlite3_errmsg(db));
       sqlite3_close(db);
       }

       sqliteResult = sqlite3_exec(db, "pragma table_info(records);", populateColumnNames, this, &sqliteErrMsg);
       if(sqliteResult != SQLITE_OK) {
       printf("sqlite error: %s\n", sqliteErrMsg);
       sqlite3_close(db);
       }

       sqliteResult = sqlite3_exec(db, "select * from records;", populateRows, this, &sqliteErrMsg);
       if(sqliteResult != SQLITE_OK) {
       printf("sqlite error: %s\n", sqliteErrMsg);
       sqlite3_close(db);
       }
       */

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

    queryInput->performQuery();

    Fl::add_fd(sockfd, FL_READ, handleFD, (void*)this);
}

int TableView::handle(int event) {
    switch(event) {
    case FL_KEYDOWN:
    case FL_SHORTCUT: {
        int key = Fl::event_key();
        if(key == ' ') {
            //liveMode = true;
        }
    }
    default:
        return Fl_Scroll::handle(event);
    }
}

void TableView::enableWidget(int row, int col, const char *label) {
    Fl_Widget *widget = widgets[row][col];
    widget->box(FL_BORDER_BOX);
    if(row > 0) {
        widget->color(FL_WHITE);
    }
    widget->copy_label(label);
    add(widget);
}


