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
        if ( isxdigit(*ap) && isxdigit(*bp) ) {           // cheezy detection of numeric data
            // Numeric sort
            int av=0; sscanf(ap, "%x", &av);
            int bv=0; sscanf(bp, "%x", &bv);
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
    Table(int x, int y, int w, int h, const char *l=0);
    ~Table();
    void add_row(Row &newRow);
    void autowidth(int pad);
    void resize_window();
    void remove_row(int index);
    void remove_last_row();
    int  minimum_row(unsigned int column_index);
    void clear();
    void clearNewQueries();
    void changeSort();
    void sortUI();
    void done();

    static void scrollToRow(int row, void *data);

    float ** values;
    std::vector<const char *>* headers;
    std::vector<Fl_Widget*> sparklines;
    std::vector<Fl_Window*>* spawned_windows;
    QueryInput *queryInput;
    int readyToDraw;
    std::vector<Row> rowdata;
    Fl_Widget *dummy;
};


