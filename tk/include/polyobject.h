class PolyObject : public SiteObject {
public:
    PolyObject();
    virtual ~PolyObject();

    void init(float worldX, float worldY, float r, float g, float b);

    void drawWorld();
    void drawScreen(bool drawCenterPoint, float cellsPerMeter, float pixelsPerCell, float worldPanX, float worldPanY);
    void drawPolyLine(float screenOffsetX, float screenOffsetY, float pixelsPerCell, float cellsPerMeter);

    float getWorldOffsetCenterX();
    float getWorldOffsetCenterY();

    void updateWorldOffsetCenterX(float offsetCenterX);
    void updateWorldOffsetCenterY(float offsetCenterY);

    std::string toString();
    bool fromString(char* output);

    void addPoint(float x, float y);
    void setNextPoint(float x, float y);
    void updateSize(float worldDifferenceX, float worldDifferenceY);
    void confirm();
    void cancel();

    int numPoints();

    struct Point {
        float x;
        float y;

        Point() {
            x = 0.0;
            y = 0.0;
        }

        Point(float x1, float y1) {
            x = x1;
            y = y1;
        }

        ~Point() { };
    };

private:

    void calculateCenterPoint();

    std::vector<Point> points;
    Point* nextPoint;

    bool recalculateCenterPoint;

    float centerPointWorldOffsetX;
    float centerPointWorldOffsetY;

};
