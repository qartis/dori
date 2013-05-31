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

    Toolbar *toolbar;

private:

    float scaledSelectionDistance();
    static int sqlite_cb(void *arg, int ncols, char**cols, char **colNames);

    void processData(const char *filename, const char *db_name);
    SiteObject* closestGeom(int screenX, int screenY, std::vector<SiteObject*> &dataset, int *distance = NULL, bool clearColours = false);

    CircleObject* curSelectedPoint;

    float minX;
    float minY;
    float maxX;
    float maxY;
    float minElevation;
    float maxElevation;

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
    SiteObject *newSiteObject;

    sqlite3 *db;

    float maxScreenWidth;
    float maxScreenHeight;
};