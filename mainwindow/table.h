//
// Sort class to handle sorting column using std::sort
class SortColumn {
    int _col, _reverse;
public:
    SortColumn(int col, int reverse) {
        _col = col;
        _reverse = reverse;
    }
    bool operator()(const Row &a, const Row &b) {
        const char *ap = ( _col < (int)a.cols.size() ) ? a.cols[_col] : "",
              *bp = ( _col < (int)b.cols.size() ) ? b.cols[_col] : "";
        if ( isdigit(*ap) && isdigit(*bp) ) {           // cheezy detection of numeric data
            // Numeric sort
            int av=0; sscanf(ap, "%d", &av);
            int bv=0; sscanf(bp, "%d", &bv);
            return( _reverse ? av < bv : bv < av );
        } else {
            // Alphabetic sort
            return( _reverse ? strcmp(ap, bp) > 0 : strcmp(ap, bp) < 0 );
        }
    }
};

// Derive a custom class from Fl_Table_Row
class Table : public Fl_Table_Row {
private:
    int _sort_reverse;
    int _sort_lastcol;
    int _sort_curcol;

    static void event_callback(Fl_Widget*, void*);
    void event_callback2();                                     // callback for table events

protected:
    void draw_cell(TableContext context, int R=0, int C=0,      // table cell drawing
                   int X=0, int Y=0, int W=0, int H=0);
    void sort_column(int col, int reverse=0);                   // sort table by a column
    void draw_sort_arrow(int X,int Y,int W,int H,int sort);
    virtual void draw();
    void redrawSpawnables();

public:
    // Ctor
    Table(int x, int y, int w, int h, const char *l=0) : Fl_Table_Row(x,y,w,h,l) {
        _sort_reverse = 1;
        _sort_lastcol = 5; // set these to 4 by default (Timestamp)
        _sort_curcol = 5;
        readyToDraw = 0;
        end();
        callback(event_callback, (void*)this);
    }
    ~Table() { }
    void add_row(const char *row, bool greenify, bool perform_autowidth);
    void autowidth(int pad);
    void resize_window();
    void remove_row(int index);
    void remove_last_row();
    int  minimum_row(unsigned int column_index);
    void clear();
    void clearNewQueries();
    void changeSort();
    void sortUI();

    std::vector<const char *>* headers;
    std::vector<Fl_Window*>* spawned_windows;
    QueryInput *queryInput;
    int readyToDraw;
    std::vector<Row> _rowdata;

    int totalRows() { return _rowdata.size(); }

};


