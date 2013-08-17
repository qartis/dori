class Toolbar : public Fl_Window {

public:
    Toolbar(int x, int y, int w, int h, const char *label = NULL);

    void (*setColorCallback)(void *data, unsigned r, unsigned g, unsigned b);
    void (*clickedObjTypeCallback)(void *data);

    void clearSelectedObjectType();
    virtual int handle(int event);
    virtual void draw();

    Fl_Button *lineButton;
    Fl_Button *rectButton;
    Fl_Button *circleButton;
    Fl_Button *polyButton;
    ColorChooser *colorChooser;

    Fl_Tree *tree;

    SiteObjectType curSelectedObjType;
};
