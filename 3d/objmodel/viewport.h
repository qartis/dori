struct dataPoint {
    unsigned long time;
    char dataString[256];
};

class Viewport : public FlGlArcballWindow {
public:
    Viewport(int x, int y, int w, int h, const char*L=0);

    virtual void draw();
    void addModel(ObjModel& model);
    ObjModel* getModel(const char *filename);

    void insertDataSorted(char * s);
    std::list<dataPoint> dataList;
    std::list<dataPoint>::iterator dataIterator;
    std::vector<Row>* rowData;

private:
    virtual int handle(int event);

    std::vector<ObjModel> models;
    fl_gl_contour *contour;
    int graphType2d;
    int graphType3d;

    int gridXCells;
    int gridYCells;
    int gridZCells;

    int robotModelIndex;
};
