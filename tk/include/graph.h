class Graph : Fl_Double_Window {

    Graph(int x, int y, int w, int h, const char *label = NULL);


    virtual void resize(int x, int y, int w, int h);
    virtual void draw();
};
