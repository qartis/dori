class SiteEditor : public Fl_Double_Window {

public:

    SiteEditor(int x, int y, int w, int h, const char *label = NULL);

    ~SiteEditor();

    void insertDataPoint(int index, float distance);

    // overridden functions
    virtual int handle(int event);
    virtual void draw();

    std::vector<SiteObject*> points;
    std::vector<SiteObject*> siteObjects;

private:

    static int sqlite_cb(void *arg, int ncols, char**cols, char **colNames);

    void processData(const char *filename);
    SiteObject* closestGeom(int screenX, int screenY, std::vector<SiteObject*> &dataset, int *distance = NULL, bool clearColours = false);

    CircleObject* curSelectedPoint;

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

    SiteObject *curSelectedObject;

    sqlite3 *db;
    Toolbar *toolbar;

    SiteObject *newSiteObject;
};
