#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Table.H>
#include <FL/Fl_Table_Row.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Gl_Window.H>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <string.h>
#include "ctype.h"
#include "../row.h"
#include "queryinput.h"
#include "table.h"

#define MARGIN 20

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
    if ( !_sort_reverse ) {
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
                fl_draw((*headers)[C], X+2,Y,W,H, FL_ALIGN_LEFT, 0, 0);         // +2=pad left 
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
            Fl_Color bgcolor;

            if(_rowdata[R].is_live_data)
            {
                bgcolor = row_selected(R) ? selection_color() : FL_GREEN;
            }
            else {
                bgcolor = row_selected(R) ? selection_color() : FL_WHITE;
            }
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

void Table::autowidth(int pad) {
    fl_font(FL_COURIER, 16); 
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

void Table::add_row(const char *row, bool greenify) {
    char s[512];
    Row newrow;

    newrow.is_live_data = greenify;

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

    redrawSpawnables();
}

void Table::remove_row(int index) {
    _rowdata.erase(_rowdata.begin() + index);
    rows(rows() - 1);

    redrawSpawnables();
}

void Table::remove_last_row() {
    remove_row(_rowdata.size() - 1);
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

void Table::clearNewQueries() {
    std::vector<Row>::iterator it = _rowdata.begin();
    for(; it != _rowdata.end(); it++) {
        it->is_live_data = false;
    }

    redrawSpawnables();
    redraw();
}

void Table::redrawSpawnables() {
    std::vector<Fl_Window*>::iterator it;
    for(it = spawned_windows->begin(); it != spawned_windows->end(); it++) {
        (*it)->redraw();
    }
}

void Table::event_callback(Fl_Widget*, void *data) {
    Table *o = (Table*)data;
    o->event_callback2();
}

void Table::changeSort() {
    if(!readyToDraw) {
        return;
    }

    char buf[256];
    char str[256];
    char *ptr;

    strcpy(str, queryInput->value());

    ptr = strstr(str, "order by ");

    if(ptr) {
        strncpy(buf, str, ptr - str);
        sprintf(buf + (ptr - str), "order by %s %s", (*headers)[_sort_curcol], _sort_reverse ? "asc" : "desc");

        /*char *order = */strtok(ptr, " ");
        char *by = strtok(NULL, " ");
        char *space = strchr(by + strlen("by "), ' ');
        char *colname = strtok(NULL, " ");
        char *ascdesc = colname + strlen(colname);

        if (space) {
            ascdesc++;
        }

        if (*ascdesc == '\0') {
        } else if(strncasecmp(ascdesc, "desc", strlen("desc")) == 0){
            strcat(buf, ascdesc + strlen("desc"));
        }else if(strncasecmp(ascdesc, "asc", strlen("asc")) == 0){
            strcat(buf, ascdesc + strlen("asc"));
        } else {
            strcat(buf, " ");
            strcat(buf, ascdesc);
        }
    }
    else {
        ptr = strstr(str, "limit ");
        if (ptr) {
            strncpy(buf, str, ptr - str);
            sprintf(buf + (ptr - str), "order by %s %s", (*headers)[_sort_curcol], _sort_reverse ? "asc" : "desc");
            strcat(buf, " ");
            strcat(buf, ptr);
        }
        else {
            ptr = strstr(str, ";");

            if(ptr) {
                strncpy(buf, str, ptr - str);
                sprintf(buf + (ptr - str), " order by %s %s", (*headers)[_sort_curcol], _sort_reverse ? "asc" : "desc");
            }
            else {
                sprintf(buf, "%s order by %s %s", str, (*headers)[_sort_curcol], _sort_reverse ? "asc" : "desc");
            }
        }
    }

    queryInput->value(buf);
    queryInput->performQuery();

    _sort_lastcol = _sort_curcol;
}

void Table::sortUI() {
    sort_column(_sort_curcol, _sort_reverse);
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
                _sort_reverse = 1;
            }
            changeSort();
        }
        break;
    }
    default:
        return;
    }
}

