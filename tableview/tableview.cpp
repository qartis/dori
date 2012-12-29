#include "tableview.h"
#include <FL/fl_draw.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Box.H>
#include <math.h>
#include <sqlite3.h>

static char* columnLookupQuery = "pragma table_info(records);";
static char* defaultQuery = "select * from records;";

static int populateColumnNames(void *tableview, int argc, char **argv, char **colName){
    TableView *tview = (TableView*)tableview;
    for(int i=0; i < argc; i++){
        if(strcmp(colName[i],"name") == 0) {
            Fl_Widget *widget = tview->widgets[0][tview->totalCols];
            widget->box(FL_BORDER_BOX);
            widget->copy_label(argv[i]);
            tview->add(widget);
            tview->totalCols++;
        }
    }

    tview->totalRows = 1;
    return 0;
}

static int populateRows(void *tableview, int argc, char **argv, char **colName){
    TableView *tview = (TableView*)tableview;
    for(int i=0; i < argc; i++){
        Fl_Widget *widget = tview->widgets[tview->totalRows][i];
        widget->box(FL_BORDER_BOX);
        widget->color(FL_WHITE);
        widget->copy_label(argv[i]);
        tview->add(widget);
    }
    tview->totalRows++;
    return 0;
}

static void performQuery(void *tableview, void *str) {
    TableView *tview = (TableView*)tableview;
    // clear out the old results
    for(int i = 0; i < tview->totalRows; i++) {
        for(int j = 0; j < tview->totalCols; j++) {
            tview->remove(tview->widgets[i][j]);
        }
    }

    tview->totalRows = tview->totalCols = 0;

    char *sqliteErrMsg = 0;
    int sqliteResult = 0;

    tview->queryInput->color(FL_WHITE);

    sqliteResult = sqlite3_exec(tview->db, columnLookupQuery, populateColumnNames, tview, &sqliteErrMsg);
    if(sqliteResult != SQLITE_OK) {
        printf("sqlite error: %s\n", sqliteErrMsg);
        tview->queryInput->color(FL_RED);
    }

    sqliteResult = sqlite3_exec(tview->db, (char *)str, populateRows, tview, &sqliteErrMsg);
    if(sqliteResult != SQLITE_OK) {
        printf("sqlite error: %s\n", sqliteErrMsg);
        tview->queryInput->color(FL_RED);
    }

    tview->redraw();
}


TableView::TableView(int x, int y, int w, int h, const char *label) : totalRows(0), totalCols(0), Fl_Scroll(x, y, w, h, label) {
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

    //sqlite3_close(db);

    /*
    int cellw = 80;
    int cellh = 25;
    int xx = x, yy = y;
    Fl_Tile *tile = new Fl_Tile(x,y,cellw*cols,cellh*rows);
    // Create widgets
    for ( int r=0; r<rows; r++ ) {
        for ( int c=0; c<cols; c++ ) {
            if ( r==0 ) {
                Fl_Box *box = new Fl_Box(xx,yy,cellw,cellh,header[c]);
                box->box(FL_BORDER_BOX);
                entries[r][c] = (void*)box;
            } else if ( c==0 ) {
                Fl_Input *in = new Fl_Input(xx,yy,cellw,cellh);
                in->box(FL_BORDER_BOX);
                in->value("");
                entries[r][c] = (void*)in;
            } else {
                Fl_Float_Input *in = new Fl_Float_Input(xx,yy,cellw,cellh);
                in->box(FL_BORDER_BOX);
                in->value("0.00");
                entries[r][c] = (void*)in;
            }
            xx += cellw;
        }
        xx = x;
        yy += cellh;
    }
    tile->end();
    end();
    */
}

int TableView::handle(int event) {
    switch(event) {
    case FL_KEYDOWN:
    case FL_SHORTCUT: {
        int key = Fl::event_key();
        /*
           if(key == 'h') {
           int startPoint = curPointIndex;
           do {
           curPointIndex++;

           if(curPointIndex > MAX_ANGLES) {
           curPointIndex = 0;
           }
           } while(!data[curPointIndex].valid && curPointIndex != startPoint);
           redraw();
           return 1;
           }
           */
    }
    default:
        return Fl_Scroll::handle(event);
    }
}
