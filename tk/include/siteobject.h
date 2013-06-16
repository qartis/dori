enum SiteObjectType {
    UNDEFINED,
    LINE,
    RECT,
    CIRCLE,
};

#define MAX_METER_EXTENTS 100.0

class SiteObject {
public:

    static float screenToWorld(int screenVal, float meterExtents, int pixelExtents);
    static int worldToScreen(int worldVal, float meterExtents, int pixelExtents);

    virtual void drawWorld() = 0;
    virtual void drawScreen(bool drawCenterPoint, int screenWidth, int screenHeight, float siteMeterExtents) = 0;

    virtual float getWorldOffsetCenterX() = 0;
    virtual float getWorldOffsetCenterY() = 0;

    virtual void setWorldOffsetCenterX(float offsetCenterX) = 0;
    virtual void setWorldOffsetCenterY(float offsetCenterY) = 0;

    virtual void toString(char*) = 0;
    virtual void fromString(char*) = 0;

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

    SiteObjectType type;
};
