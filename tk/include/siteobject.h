enum SiteObjectType {
    UNDEFINED = 0,
    LINE,
    RECT,
    CIRCLE,
    POLY,
};

#define SITE_METER_EXTENTS 80.0

class SiteObject {
public:

    static float screenToWorld(float screenVal, float cellsPerPixel, float metersPerCell);
    static float worldToScreen(float worldVal, float pixelsPerCell, float cellsPerMeter);

    virtual void drawWorld() = 0;
    virtual void drawScreen(bool drawCenterPoint, float cellsPerMeter, float pixelsPerCell, float worldPanX, float worldPanY) = 0;

    virtual float getWorldOffsetCenterX() = 0;
    virtual float getWorldOffsetCenterY() = 0;

    virtual void updateWorldOffsetCenterX(float offsetCenterX) = 0;
    virtual void updateWorldOffsetCenterY(float offsetCenterY) = 0;

    virtual void updateSize(float worldOffsetX, float worldOffsetY) = 0;
    virtual void confirm() = 0;
    virtual void cancel() = 0;

    virtual std::string toString() = 0;
    virtual void fromString(char*) = 0;

    virtual void init(float worldX, float worldY, float r, float g, float b) = 0;

    void setRGB(unsigned red, unsigned green, unsigned blue);

    void select();

    void unselect();

    SiteObject();
    virtual ~SiteObject() { }

    int id;

    float elevation;
    float worldHeight;

    unsigned r;
    unsigned g;
    unsigned b;

    unsigned backupR;
    unsigned backupG;
    unsigned backupB;

    char special[128];

    bool selected;

    float worldOffsetX;
    float worldOffsetY;

    float siteCenterX;
    float siteCenterY;

    SiteObjectType type;
};
