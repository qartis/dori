#include <FL/Fl.H>
#include <FL/Fl_Input.H>

class QueryInput : public Fl_Input {

public:
    QueryInput(int x, int y, int w, int h, const char *label = NULL);

    virtual int handle(int event);
    void performQuery(void);

    void (*callback)(void*);

    bool isLiveMode();
    char* getSearchString();

    int getLimit();

};
