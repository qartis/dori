#include <FL/Fl.H>
#include <FL/Fl_Input.H>

class TableInput : public Fl_Input {

public:
    TableInput(int x, int y, int w, int h, const char *label = NULL);

    // overridden functions
    virtual int handle(int event);

    void (*callback)(void*, void*);

private:
};
