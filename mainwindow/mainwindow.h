#define BUFLEN 128

class MainWindow : public Fl_Window {

public:
    MainWindow(int x, int y, int w, int h, const char *label = NULL);

    virtual int handle(int event);
    void enableWidget(int row, int col, const char *label = NULL);

    Table* table;
    sqlite3 *db;
    sqlite3 *db_tmp;
    QueryInput *queryInput;
    char buffer[BUFLEN];
    int bufMsgStartIndex;
    int bufReadIndex;
    int sockfd;

    WidgetWindow *widgetWindow;

    std::vector<Fl_Window*> spawned_windows;
    static int sqlite_cb(void *arg, int ncols, char **cols, char **rows);
    static void performQuery(void *arg);

    void clearTable(void *arg);
    bool needFlush;

    bool greenify;
    
    std::vector<const char *> headers;
};
