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

    static void setColorCallback(void *data, float r, float g, float b);
    static void clickedObjTypeCallback(void *data);

    sqlite3 *db;

    void selectAllObjects();

    void clearSelectedObjects();

    virtual void resize(int X, int Y, int W, int H);

    typedef enum {
        WAITING,
        DRAWING,
        SELECTED,
        SELECTING,
    } state;

private:

    float scaledSelectionDistance();
    static int sqlite_cb(void *arg, int ncols, char**cols, char **colNames);

    void loadSiteObjects(const char *db_name);

    SiteObject* findClosestObject(int mouseX, int mouseY, int distance, std::vector<SiteObject*> &dataset);

    void findObjectsInBoundingBox(int startMouseX, int startMouseY, int curMouseX, int curMouseY, std::vector<SiteObject*> &inputData, std::vector<SiteObject*> &outputData);

    void drawGrid();
    void drawArcs();
    void drawDori();
    void drawCrosshair();
    void drawSelectionState();
    void drawSiteObjects();

    void drawDistanceText();
    void drawArrow(float arrowScreenX, float arrowScreenY, float arrowAngleDegrees);

    void calcArrowScreenPosition(float centerX,
                                float centerY,
                                float doriX,
                                float doriY,
                                float& arrowX,
                                float& arrowY);

    float calcArrowAngleDegrees(float arrowScreenX,
                                     float arrowScreenY,
                                     float centerX,
                                     float centerY,
                                     float doriX,
                                     float doriY);

    void calcArcInfo();

    void createNewObject(SiteObjectType type, float mouseX, float mouseY);

    void processNewSiteObject();

    void commitSelectedObjectsToDatabase();

    void enforceSiteBounds();

    float screenToWorld(float screenVal);
    float worldToScreen(float worldVal);

    float screenCenterWorldX();
    float screenCenterWorldY();

    bool isObjectSelected(SiteObject *obj);

    void panToWorldCenter();
    void handlePan();
    void handleMouseMovement(int mouseX, int mouseY);

    int handleLeftMouse(int event,
                        int curMouseX,
                        int curMouseY,
                        int& clickedMouseX,
                        int& clickedMouseY);

    int handleKeyPress(int key,
                       int centerX,
                       int centerY,
                       int doriX,
                       int doriY);

    int handleMouseWheel(float centerX,
                         float centerY,
                         float doriX,
                         float doriY,
                         float arrowScreenX,
                         float arrowScreenY);


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

    state curState;

    SiteObject *newSiteObject;

    float maxWindowWidth;
    float maxWindowLength;

    float siteMeterExtents;
    int sitePixelExtents;
    int numGridCells;

    // Keeps track of where we've panned the screen to
    float panStartWorldX;
    float panStartWorldY;

    float worldPanX;
    float worldPanY;

    float pixelsPerCell;
    float cellsPerMeter;

    int numArcs;

    bool showArcs;
    bool showGrid;

    float curWorldDistance;

    float arrowScreenX;
    float arrowScreenY;

    float arrowAngleDegrees;

    struct ArcInfo {
        bool isOnScreen;
        float screenRadius;
    };

    std::vector<ArcInfo> arcInfo;
};
