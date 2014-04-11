#define BUFLEN 128

class MainWindow : public Fl_Window {

public:
    MainWindow(int x, int y, int w, int h, const char *label = NULL);

    virtual int handle(int event);
    void enableWidget(int row, int col, const char *label = NULL);

    Table* table;
    sqlite3 *db;
    QueryInput *queryInput;
    char buffer[BUFLEN];
    int bufMsgStartIndex;
    int bufReadIndex;
    int sockfd;

    WidgetWindow *widgetWindow;

    std::vector<Fl_Window*> spawned_windows;
    static int sqlite_cb(void *arg, int ncols, char **cols, char **rows);

    static void testQuery(void *arg);
    static void performQuery(void *arg);

    void clearTable(void *arg);
    bool needFlush;

    bool receivedFirstDump;

    std::vector<const char *> headers;
    std::vector<const char *> prev_queries;

    FILE *shell_log;
    static const char* shell_log_filename;
    QueryTemplateWidget *queryTemplateWidget;
};
