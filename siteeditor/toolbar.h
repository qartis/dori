enum geomType {
    NONE = 0,
    LINE,
    RECT,
    CIRCLE,
};

class Toolbar : public Fl_Window {

public:
    Toolbar(int x, int y, int w, int h, const char *label = NULL);

    virtual int handle(int event);

    Fl_Button *lineButton;
    Fl_Button *rectButton;
    Fl_Button *circleButton;

    geomType curSelectedType;
};
