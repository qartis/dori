enum objType {
    UNDEFINED,
    LINE,
    RECT,
    CIRCLE,
};

class SiteObject {
public:

    float scaledScreenCenterX(int curScreenWidth, int maxScreenWidth) {
        return ((float)screenCenterX / (float)maxScreenWidth) * (float)curScreenWidth;
    }
    float scaledScreenCenterY(int curScreenHeight, int maxScreenHeight) {
        return ((float)screenCenterY / (float)maxScreenHeight) * (float)curScreenHeight;
    }

    virtual void drawWorld() = 0;
    virtual void drawScreen(bool drawCenterPoint, float screenWidth, float screenMaxWidth, float screenHeight, float screenMaxHeight) = 0;
    virtual void calcWorldCoords(float scaleX, float scaleY, float minX, float minY) = 0;
    virtual void scaleWorldCoords(float midX, float minY, float midY, float maxY) = 0;
    virtual void moveCenter(int newCenterX, int newCenterY) = 0;

    virtual void toString(char*) = 0;
    virtual void fromString(char*) = 0;

    virtual ~SiteObject() { }

    int id;

    int screenCenterX;
    int screenCenterY;

    float elevation;
    float worldHeight;

    unsigned r;
    unsigned g;
    unsigned b;

    char special[128];

    objType type;
};
