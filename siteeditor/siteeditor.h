typedef struct {
    unsigned id;
    int screenX;
    int screenY;
    int screenWidth;
    int screenHeight;

    int screenCenterX;
    int screenCenterY;

    float worldX;
    float worldY;
    float worldWidth;
    float worldHeight;

    float elevation;
    unsigned r;
    unsigned g;
    unsigned b;
} point;

class SiteEditor : public Fl_Double_Window {

public:

    SiteEditor(int x, int y, int w, int h, const char *label = NULL);

    ~SiteEditor();

    void insertDataPoint(int index, float distance);

    // overridden functions
    virtual int handle(int event);
    virtual void draw();

    std::vector<point> points;
    std::vector<point> siteObjects;

private:

    static int sqlite_cb(void *arg, int ncols, char**cols, char **colNames);

        void processData(const char *filename);
    point* closestPoint(int screenX, int screenY, std::vector<point> &dataset, int *distance = NULL, bool clearColours = false);

    point* curSelectedPoint;

    float minX;
    float minY;
    float minElevation;

    float scaleX;
    float scaleY;

    // TODO: refactor these into a 'point' struct
    int newSquareOriginX;
    int newSquareOriginY;

    int newSquareCenterX;
    int newSquareCenterY;

    int newSquareWidth;
    int newSquareHeight;

    int newSquareR;
    int newSquareG;
    int newSquareB;

    float curMouseOverElevation;

    typedef enum {
        WAITING,
        DRAWING,
        SELECTED,
    } state;

    state curState;

    point *curSelectedSquare;

    sqlite3 *db;

};
