class RectObject : public SiteObject {
public:
    RectObject() :
        screenLeft(0), screenTop(0),
        screenWidth(0), screenHeight(0),
        worldLeft(0.0), worldTop(0.0),
        worldWidth(0.0), worldLength(0.0)
    {
        type = RECT;
    }

    ~RectObject() { };

    void drawWorld();
    void drawScreen(bool drawCenterPoint, float windowWidth, float windowMaxWidth, float windowHeight, float windowMaxHeight);
    void calcWorldCoords(float scaleX, float scaleY, float minX, float minY);
    void moveCenter(int newCenterX, int newCenterY);
    void scaleWorldCoords(float midX, float minY, float midY, float maxY);

    void toString(char* input);
    void fromString(char* output);

    int screenLeft;
    int screenTop;

    int screenWidth;
    int screenHeight;

    float worldLeft;
    float worldTop;

    float worldWidth;
    float worldLength;
};
