enum objType {
    UNDEFINED,
    LINE,
    RECT,
    CIRCLE,
};

class SiteObject {
public:
    virtual void drawWorld() = 0;
    virtual void drawScreen(bool drawCenterPoint = true) = 0;
    virtual void calcWorldCoords(float scaleX, float scaleY, float minX, float minY) = 0;
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

    objType type;
};
