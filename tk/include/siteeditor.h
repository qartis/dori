class SiteEditor : public Fl_Double_Window {

public:

    SiteEditor(int x, int y, int w, int h, const char *label = NULL);

    ~SiteEditor();

    void insertDataPoint(int index, float distance);

    // overridden functions
    virtual int handle(int event);
    virtual void draw();

    std::vector<SiteObject*> siteObjects;
    std::vector<SiteObject*> selectedObjects;

    Toolbar *toolbar;

    void beginDatabaseTransaction();
    void endDatabaseTransaction();

    sqlite3 *db;

    void selectAllObjects();

    void clearSelectedObjects();

private:

    float scaledSelectionDistance();
    static int sqlite_cb(void *arg, int ncols, char**cols, char **colNames);

    void loadSiteObjects(const char *db_name);

    SiteObject* findClosestObject(int mouseX, int mouseY, int distance, std::vector<SiteObject*> &dataset);

    void findObjectsInBoundingBox(int startMouseX, int startMouseY, int curMouseX, int curMouseY, std::vector<SiteObject*> &inputData, std::vector<SiteObject*> &outputData);

    void drawGrid(float pixelsPerCell, int numCells);
    void drawArcs();
    void drawDori();
    void drawDistanceLine();
    void drawDistanceText();

    void createNewObject(SiteObjectType type, int mouseX, int mouseY);

    void sizeObject(SiteObjectType type, SiteObject *object, int clickedX, int clickedY, int curX, int curY);

    void commitSelectedObjectsToDatabase();

    virtual void resize(int X, int Y, int W, int H);

    float screenToWorld(float screenVal);
    float worldToScreen(float worldVal);

    int siteScreenCenterX();
    int siteScreenCenterY();

    bool isObjectSelected(SiteObject *obj);

    int clickedMouseX;
    int clickedMouseY;

    int curMouseX;
    int curMouseY;

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
        SELECTING,
    } state;

    state curState;

    SiteObject *newSiteObject;

    float maxWindowWidth;
    float maxWindowLength;

    float siteMeterExtents;
    int sitePixelExtents;
    int numGridCells;

    // Keeps track of where we've panned the screen to
    int panStartMouseX;
    int panStartMouseY;

    int screenPanX;
    int screenPanY;

    float pixelsPerCell;
    float cellsPerMeter;

    int numArcs;

    bool showArcs;
    bool showGrid;
};
