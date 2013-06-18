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

    void findObjectsInBoundingBox(float worldStartMouseX, float worldStartMouseY, float worldCurMouseX, float worldCurMouseY, std::vector<SiteObject*> &inputData, std::vector<SiteObject*> &outputData);

    void drawGrid(int numCells);

    void drawDori();

    void createNewObject(SiteObjectType type, int mouseX, int mouseY);

    void sizeObject(SiteObjectType type, SiteObject *object, int mouseX, int mouseY);

    void commitSelectedObjectsToDatabase();

    int doriScreenX();
    int doriScreenY();

    bool isObjectSelected(SiteObject *obj);

    int selectionStartMouseX;
    int selectionStartMouseY;

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

    float maxScreenWidth;
    float maxScreenHeight;

    float siteMeterExtents;
    int numGridCells;
};
