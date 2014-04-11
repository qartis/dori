class QueryTemplateWidget : public Fl_Widget {
public:
    QueryTemplateWidget(sqlite3 *db,int X,int Y,int W,int H,const char*L=0);
    virtual void draw(void);

private:

    static int sqlite_cb(void *arg, int ncols, char **cols, char **colNames);
    // Hide the default constructor
    QueryTemplateWidget(int X,int Y,int W,int H,const char*L=0) : Fl_Widget(X,Y,W,H,L) {}

    struct QueryTemplate {
        QueryTemplate() {
            sparkline = NULL;
            query = NULL;
            rowCount = 0;
        }

        Fl_Sparkline *sparkline;
        const char* query;
        int rowCount;
    };

    struct QueryRow {
        std::vector<int> cols;
    };

    std::vector<QueryTemplate*> queryTemplates;
};
