#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Table.H>
#include <FL/Fl_Table_Row.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Button.H>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <string.h>
#include "ctype.h"
#include "row.h"
#include "queryinput.h"
#include "sparkline.h"
#include "table.h"
#include <errno.h>

#define MARGIN 20

Table::Table(int x, int y, int w, int h, const char *l) : Fl_Table_Row(x,y,w,h,l) {
    _sort_reverse = 1;
    _sort_lastcol = 6; // set these to 5 by default (Timestamp)
    _sort_curcol = 6;
    readyToDraw = 0;
    headers = NULL;
    spawned_windows = NULL;
    col_header_height(100);
    end();
    callback(event_callback, (void*)this);
    values = NULL;
}

Table::~Table() {
}

// Sort a column up or down
void Table::sort_column(int col, int reverse) {
    std::stable_sort(rowdata.begin(), rowdata.end(), SortColumn(col, reverse));
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

    return Fl_Table_Row::draw();
}
// Handle drawing all cells in table
void Table::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) {
    const char *s = "";
    if ( R < (int)rowdata.size() && C < (int)rowdata[R].cols.size() )
        s = rowdata[R].cols[C];
    switch ( context ) {
    case CONTEXT_COL_HEADER:
        fl_push_clip(X,Y,W,H); {
            fl_draw_box(FL_THIN_UP_BOX, X,Y,W,H, FL_BACKGROUND_COLOR);
            if ( C < 9 ) {
                if(C < (int)(sparklines.size())) {
                    sparklines[C]->resize(X + 3, Y + 20, W - 6,  H - 25);
                    sparklines[C]->damage(1);
                }
                fl_font(FL_HELVETICA_BOLD, 16);
                fl_color(FL_BLACK);
                fl_draw((*headers)[C], X+2,Y,W,H, FL_ALIGN_TOP, 0, 0);
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

void Table::autowidth(int pad) {
    fl_font(FL_COURIER, 16);

    if(rowdata.size() == 0) {
        redraw();
        return;
    }

    cols((int)rowdata.back().cols.size());

    if(cols() == 0) {
        redraw();
        return;
    }

    int columns_width = 0;
    int w, h;
    for ( int c=0; c<cols(); c++ ) col_width(c, pad);
    for ( int r=0; r<(int)rowdata.size(); r++ ) {
        for ( int c=0; c<(int)rowdata[r].cols.size(); c++ ) {
            fl_measure(rowdata[r].cols[c], w, h, 0);
            if ( (w + pad) > col_width(c)) col_width(c, w + pad);
            if(r == ((int)rowdata.size() - 1)) {
                columns_width += col_width(c);
            }
        }
    }
    if(rowdata.size() > 0 && (columns_width < this->w())) {
        int difference_per_col = (this->w() - columns_width - MARGIN) / rowdata[0].cols.size();

        if(difference_per_col > 0) {
            for ( int c=0; c<cols(); c++ ) {
                col_width(c, col_width(c) + difference_per_col);
            }
        }
    }
    rows((int)rowdata.size());
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

void Table::add_row(Row &newRow) {
    rowdata.push_back(newRow);
    rows(rows() + 1);
    redrawSpawnables();
}

void Table::done() {
    if(cols() == 0)
        return;

    if(values == NULL) {
        values = new float*[cols()];
        for(int i = 0; i < cols(); i++) {
            values[i] = new float[rows()];
        }
    }

    for(int c = 0; c < cols(); c++) {
        Fl_Sparkline *sparkline = new Fl_Sparkline(0, 0, 0, 0);
        parent()->add(sparkline);
        for(int r = 0; r < rows(); r++) {
            float f = 0;
            sscanf(rowdata[r].cols[c], "%f", &f);
            values[c][r] = f;
        }
        sparkline->setValues(values[c], rows());
        sparklines.push_back(sparkline);
    }
}

void Table::remove_row(int index) {
    rowdata.erase(rowdata.begin() + index);
    rows(rows() - 1);

    redrawSpawnables();
}

void Table::remove_last_row() {
    remove_row(rowdata.size() - 1);
}

int Table::minimum_row(unsigned int column_index) {
    int min_val;;
    int min_index;

    if(rowdata.size() == 0) {
        return -1;
    }

    if(column_index >= rowdata[0].cols.size()) {
        return -1;
    }

    min_val = atoi(rowdata[0].cols[column_index]);
    min_index = 0;

    for(unsigned int i = 1; i < rowdata.size(); i++) {
        int row = atoi(rowdata[i].cols[column_index]);
        if(row < min_val) {
            min_val = row;
            min_index = i;
        }
    }

    return min_index;
}

void Table::clear() {
    for(int i = 0; i < cols(); i++) {
        delete values[i];
        delete sparklines[i];
    }
    delete values;
    values = NULL;

    rowdata.clear();
    sparklines.clear();
    rows(0);
    cols(0);
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
    queryInput->performQuery(queryInput->parent());

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
