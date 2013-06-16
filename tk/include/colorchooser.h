class ColorChooser : public Fl_Color_Chooser {

public:

    ColorChooser(int x, int y, int w, int h, const char *label = NULL);

    void (*setColorCallback)(void *data, float r, float g, float b);

private:
    virtual int handle(int event);

    // Using this as a hack to detect mouse release
    // can't figure out why handle()
    // won't capture an FL_RELEASE after I let
    // the base class handle the push
    //
    // The doc: http://www.fltk.org/documentation.php/doc-1.1/events.html
    // states that you just need to return a nonzero value for
    // the FL_PUSH event, in order to capture the FL_RELEASE
    bool mousePushedLastHandle;
};
