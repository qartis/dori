class QueryInput : public Fl_Input {

public:
    QueryInput(int x, int y, int w, int h, const char *label = NULL);

    virtual int handle(int event);

    void (*performQuery)(void*);
    void (*testQuery)(void*);

    char* getSearchString();

    int getLimit();
};
