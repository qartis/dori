class SiteEditor : public Fl_Double_Window {

public:

    SiteEditor(int x, int y, int w, int h, const char *label = NULL);

    ~SiteEditor();

    void insertDataPoint(int index, float distance);

    // overridden functions
    virtual int handle(int event);
    virtual void draw();

    std::vector<Geometry*> points;
    std::vector<Geometry*> siteGeoms;

private:

    static int sqlite_cb(void *arg, int ncols, char**cols, char **colNames);

    void processData(const char *filename);
    Geometry* closestGeom(int screenX, int screenY, std::vector<Geometry*> &dataset, int *distance = NULL, bool clearColours = false);

    Circle* curSelectedPoint;

    float minX;
    float minY;
    float minElevation;

    float scaleX;
    float scaleY;

    float curMouseOverElevation;

    typedef enum {
        WAITING,
        DRAWING,
        SELECTED,
    } state;

    state curState;

    Geometry *curSelectedGeom;

    sqlite3 *db;
    Toolbar *toolbar;

    Geometry *newGeom;
};
