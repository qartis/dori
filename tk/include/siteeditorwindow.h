class SiteEditorWindow : public Fl_Window {

public:

    SiteEditorWindow(int x, int y, int w, int h, const char *label = NULL);

    ~SiteEditorWindow();

    virtual void resize(int x, int y, int w, int h);

private:

    SiteEditor *siteEditor;
    Toolbar *toolbar;
};
