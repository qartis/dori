#include "queryinput.h"
#include "table.h"

#define MARGIN 20

static const char *head[] = { "rowid", "type", "a", "b", "c", "time" };

// Sort a column up or down
void Table::sort_column(int col, int reverse) {
    std::stable_sort(_rowdata.begin(), _rowdata.end(), SortColumn(col, reverse));

    redraw();
}

// Draw sort arrow
void Table::draw_sort_arrow(int X,int Y,int W,int H,int sort) {
    (void)sort;
    int xlft = X+(W-6)-8;
    int xctr = X+(W-6)-4;
    int xrit = X+(W-6)-0;
    int ytop = Y+(H/2)-4;
    int ybot = Y+(H/2)+4;
    if ( _sort_reverse ) {
        // Engraved down arrow
        fl_color(FL_WHITE);
        fl_line(xrit, ytop, xctr, ybot);
        fl_color(41);                   // dark gray
        fl_line(xlft, ytop, xrit, ytop);
        fl_line(xlft, ytop, xctr, ybot);
    } else {
        // Engraved up arrow
        fl_color(FL_WHITE);
        fl_line(xrit, ybot, xctr, ytop);
        fl_line(xrit, ybot, xlft, ybot);
        fl_color(41);                   // dark gray
        fl_line(xlft, ybot, xctr, ytop);
    }
}

void Table::draw() {
    if(!readyToDraw) {
        return;
    }

    /*
    printf("======================================================\n");
    for(unsigned i = 0; i < _rowdata.size(); i++) {
        printf("%s\n", _rowdata[i].col_str);
    }
    printf("======================================================\n\n\n");
    */


    return Fl_Table_Row::draw();
}
// Handle drawing all cells in table
void Table::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) {
    const char *s = "";
    if ( R < (int)_rowdata.size() && C < (int)_rowdata[R].cols.size() )
        s = _rowdata[R].cols[C];
    switch ( context ) {
    case CONTEXT_COL_HEADER:
        fl_push_clip(X,Y,W,H); {
            fl_draw_box(FL_THIN_UP_BOX, X,Y,W,H, FL_BACKGROUND_COLOR);
            if ( C < 9 ) {
                fl_font(FL_HELVETICA_BOLD, 16);
                fl_color(FL_BLACK);
                fl_draw(head[C], X+2,Y,W,H, FL_ALIGN_LEFT, 0, 0);         // +2=pad left 
                // Draw sort arrow
                if ( C == _sort_lastcol ) {
                    draw_sort_arrow(X,Y,W,H, _sort_reverse);
                }
            }
        }
        fl_pop_clip();
        return;
    case CONTEXT_CELL: {
        fl_push_clip(X,Y,W,H); {
            Fl_Color bgcolor = row_selected(R) ? selection_color() : FL_WHITE;
            fl_color(bgcolor); fl_rectf(X,Y,W,H); 
            fl_font(FL_HELVETICA, 16);
            fl_color(FL_BLACK); 
            fl_draw(s, X+2,Y,W,H, FL_ALIGN_LEFT);     // +2=pad left 
            fl_color(FL_LIGHT2); fl_rect(X,Y,W,H);
        }
        fl_pop_clip();
        return;
    }
    default:
        return;
    }
}

// Automatically set column widths to widest data in each column
void Table::autowidth(int pad) {
    fl_font(FL_COURIER, 16); 
    // Initialize all column widths to lowest value
    for ( int c=0; c<cols(); c++ ) col_width(c, pad);
    for ( int r=0; r<(int)_rowdata.size(); r++ ) {
        int w, h;
        for ( int c=0; c<(int)_rowdata[r].cols.size(); c++ ) {
            fl_measure(_rowdata[r].cols[c], w, h, 0);           // get pixel width of text
            if ( (w + pad) > col_width(c)) col_width(c, w + pad);
        }
    }
    table_resized();
    redraw();
}

// Resize parent window to size of table
void Table::resize_window() {
    // Determine exact outer width of table with all columns visible
    int width = 4;                                          // width of table borders
    for ( int t=0; t<cols(); t++ ) width += col_width(t);   // total width of all columns
    width += MARGIN*2;
    if ( width < 200 || width > Fl::w() ) return;
    window()->resize(window()->x(), window()->y(), width, window()->h());  // resize window to fit
}

void Table::add_row(const char *row) {
    char s[512];
    // Add a new row
    Row newrow;
    // store a delimited version of the row string
    strcpy(newrow.col_str, row);
    _rowdata.push_back(newrow);
    std::vector<char*> &rc = _rowdata.back().cols;
    char *ss;
    const char *delim = "\t";
    strcpy(s, row);
    for(int t=0; (t==0)?(ss=strtok(s,delim)):(ss=strtok(NULL,delim)); t++) {
        rc.push_back(strdup(ss));
    }
    // Keep track of max # columns
    if ( (int)rc.size() > cols() ) {
        cols((int)rc.size());
    }
    // How many rows we loaded
    rows((int)_rowdata.size());
    // Auto-calculate widths, with 20 pixel padding
    autowidth(20);
}

void Table::remove_row(int index) {
    _rowdata.erase(_rowdata.begin() + index);
    rows(rows() - 1);

    std::vector<SpawnableWindow*>::iterator it;
    for(it = spawned_windows->begin(); it != spawned_windows->end(); it++) {
        (*it)->redraw();
    }
}

void Table::remove_last_row() {
    _rowdata.erase(_rowdata.end() - 1);
    rows(rows() - 1);

    std::vector<SpawnableWindow*>::iterator it;
    for(it = spawned_windows->begin(); it != spawned_windows->end(); it++) {
        (*it)->redraw();
    }
}

int Table::minimum_row(unsigned int column_index) {
    int min_val;;
    int min_index;

    if(_rowdata.size() == 0) {
        return -1;
    }

    if(column_index >= _rowdata[0].cols.size()) {
        return -1;
    }

    min_val = atoi(_rowdata[0].cols[column_index]);
    min_index = 0;

    for(unsigned int i = 1; i < _rowdata.size(); i++) {
        int row = atoi(_rowdata[i].cols[column_index]);
        if(row < min_val) {
            min_val = row;
            min_index = i;
        }
    }

    return min_index;
}

void Table::clear() {
    _rowdata.clear();
    rows(0);
    redraw();
}

void Table::event_callback(Fl_Widget*, void *data) {
    Table *o = (Table*)data;
    o->event_callback2();
}

void Table::sort() {
    if(!readyToDraw) {
        return;
    }

    char buf[128];
    char str[128];
    char *ptr;

    //printf("queryInput->value(): '%s'\n", queryInput->value());
    strcpy(str, queryInput->value());

    ptr = strstr(str, "order by ");

    //printf("column to sort by: %d\n", _sort_curcol);
    if(ptr) {
        strncpy(buf, str, ptr - str);
        sprintf(buf + (ptr - str), "order by %s %s ", head[_sort_curcol], _sort_reverse ? "desc" : "asc");

        ptr = strtok(ptr, " ");
        ptr = strtok(NULL, " ");
        ptr = strtok(NULL, " ");
        ptr = strtok(NULL, " ");

        if(strncasecmp(ptr, "asc", strlen("asc")) == 0)
            strcpy(buf + strlen(buf), ptr + strlen("asc") + 1);
        else if(strncasecmp(ptr, "desc", strlen("desc")) == 0)
            strcpy(buf + strlen(buf), ptr + strlen("desc") + 1);
        else
            strcpy(buf + strlen(buf), ptr);
        printf("third chunk: '%s'\n", buf);
    }
    else {
        ptr = strstr(str, "limit ");
        if (ptr) {
            strncpy(buf, str, ptr - str);
            sprintf(buf + (ptr - str), "order by %s %s ", head[_sort_curcol], _sort_reverse ? "desc" : "asc");
            strcpy(buf + strlen(buf), ptr);
        }
        else {
            ptr = strstr(str, ";");

            if(ptr) {
                strncpy(buf, str, ptr - str);
                sprintf(buf + (ptr - str), " order by %s %s", head[_sort_curcol], _sort_reverse ? "desc" : "asc");
                strcpy(buf + strlen(buf), ptr);
            }
            else {
                sprintf(buf, "%s order by %s %s;", str, head[_sort_curcol], _sort_reverse ? "desc" : "asc");
                strcpy(buf + strlen(buf), ptr);
            }
        }
    }

    queryInput->value(buf);
    queryInput->performQuery();

    // sort_column(_sort_curcol, _sort_reverse);
    _sort_lastcol = _sort_curcol;
}


void Table::event_callback2() {
    int COL = callback_col();
    TableContext context = callback_context();
    switch ( context ) {
    case CONTEXT_COL_HEADER: {
        if ( Fl::event() == FL_RELEASE && Fl::event_button() == 1 ) {
            _sort_curcol = COL;
            if ( _sort_lastcol == _sort_curcol ) {
                _sort_reverse ^= 1;
            } else {
                _sort_reverse = 0;
            }
            sort();
        }
        break;
    }
    default:
        return;
    }
}

