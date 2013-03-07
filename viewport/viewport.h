typedef struct {
    float x;
    float y;
    float width;
    float length;
    float elevation;
    float r;
    float g;
    float b;
} siteObject;

class Viewport : public FlGlArcballWindow {

public:
    Viewport(int x, int y, int w, int h, const char*L=0, bool dori = false, bool showCont = false);
    ~Viewport();

    virtual void draw();
    void addModel(ObjModel& model);
    ObjModel* getModel(const char *filename);

    void insertDataSorted(char * s);

    ObjModel *dori_body;
    ObjModel *dori_arm;
    ObjModel *dori_sensor_plate;

    std::vector<Row>* rowData;

private:
    virtual int handle(int event);

    std::vector<siteObject> siteObjects;
    std::vector<ObjModel> models;
    int graphType2d;
    int graphType3d;

    int gridXCells;
    int gridYCells;
    int gridZCells;

    bool showDORI;
    bool showContour;

    FILE *objFile;
};
