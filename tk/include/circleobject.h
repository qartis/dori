class CircleObject : public SiteObject {
public:
    CircleObject() :
        screenRadius(0),
        worldCenterX(0.0), worldCenterY(0.0),
        worldRadius(0.0)
    {
        type = CIRCLE;
    }

    ~CircleObject() { }

    void scaleWorldCoords(float midX, float minY, float midY, float maxY);
    void drawWorld();
    void drawScreen(bool drawCenterPoint, float windowWidth, float windowMaxWidth, float windowHeight, float windowMaxHeight);
    void calcWorldCoords(float scaleX, float scaleY, float minX, float minY);
    void moveCenter(int newCenterX, int newCenterY);

    void toString(char* input);
    void fromString(char* output);

    int screenRadius;

    float worldCenterX;
    float worldCenterY;
    float worldRadius;
};
