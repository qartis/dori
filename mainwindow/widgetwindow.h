class WidgetWindow : public Fl_Window {

public:
    WidgetWindow(int x, int y, int w, int h, const char *label = NULL, Table *t = NULL);

    virtual int handle(int event);

    int controlFD;

    Table* table;

    Fl_Window *widgetGroup;
    Fl_Box *widgetGroupLabel;
    Fl_Button *radar;
    Fl_Button *modelViewer;
    Fl_Button *siteEditor;

    Fl_Window *graphGroup;
    Fl_Box *graphGroupLabel;
    Fl_Choice *xAxis;
    Fl_Choice *yAxis;
    Fl_Button *graph;

    Fl_Input *graphInput;
};
