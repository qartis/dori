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
#define COL1_HEADER "value1"
#define COL2_HEADER "value2"

#define PORT 1337
#define SERVER "localhost"

static char* columnLookupQuery = "pragma table_info(records);";
static char* defaultQuery = "select * from records;";

static void performQuery(void *tableview, void *str) {
    TableView *tview = (TableView*)tableview;
    char *query = (char *)str;
    printf("performing query: %s\n", query);
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
    int nbytes = read(fd, &c, 1);
    if(nbytes <= 0) {
        return;
    }

    tview->buffer[tview->bufReadIndex] = c;

    if(c == '^' || c== '!') {
        // if we're not in live mode
        // clear out the old results
        if(!tview->liveMode) {
            for(int i = 1; i < tview->totalRows; i++) {
                for(int j = 0; j < tview->totalCols; j++) {
                    tview->remove(tview->widgets[i][j]);
                }
            }
            tview->totalRows = 1;
            tview->bufReadIndex = 0;
            tview->bufMsgStartIndex = 0;
            tview->bufReadIndex = 0;
            tview->redraw();
        }
        if(c == '!') {
            tview->queryInput->color(FL_RED);
        }
        else {
            tview->queryInput->color(FL_WHITE);
        }
    }
    // start of data
    else if(c == '{') {
        tview->bufMsgStartIndex = tview->bufReadIndex+1;
    }
    else if(c == '}') {
        tview->buffer[tview->bufReadIndex] = '\0';

        // parse data
        // reset colour
        tview->queryInput->color(FL_WHITE);

        char type[BUFLEN];
        char col1[BUFLEN];
        char col2[BUFLEN];
        char col3[BUFLEN];

        buffer = &tview->buffer[tview->bufMsgStartIndex];

        // most of the stuff below is hardcoded and temporary
        char *ptr = NULL;
        ptr = strtok(buffer,",");
        strcpy(type, ptr);

        ptr = strtok(NULL,",");
        strcpy(col1, ptr);

        ptr = strtok(NULL,",");
        strcpy(col2, ptr);

        tview->enableWidget(tview->totalRows, 0, type);
        tview->enableWidget(tview->totalRows, 1, col1);
        tview->enableWidget(tview->totalRows, 2, col2);

        tview->totalRows++;

        tview->bufMsgStartIndex = 0;
        tview->bufReadIndex = 0;
        tview->redraw();
    }
    else if(c == '\0') {
        return;
    }

    tview->bufReadIndex++;
}

TableView::TableView(int x, int y, int w, int h, const char *label) : liveMode(false), bufMsgStartIndex(0), bufReadIndex(0), sockfd(0), totalRows(0), totalCols(0), Fl_Scroll(x, y, w, h, label) {
    char *sqliteErrMsg = 0;
    int sqliteResult = 0;

    queryInput = new TableInput(w * 0.2, 0, w * 0.8, CELLHEIGHT, "Search Query:");
    queryInput->callback = performQuery;
    queryInput->value(defaultQuery);

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
    printf("sockfd set: %d\n", sockfd);

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

    performQuery(this, defaultQuery);

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
    widget->color(FL_WHITE);
    widget->copy_label(label);
    add(widget);
}


