class LineObject : public SiteObject {
public:
    LineObject() :
        screenLeft(0), screenTop(0),
        screenLengthX(0), screenLengthY(0),
        worldLeft(0.0), worldTop(0.0),
        worldLengthX(0.0), worldLengthY(0.0)
    {
        type = LINE;
    }

    inline virtual ~LineObject() { };

    void scaleWorldCoords(float midX, float minY, float midY, float maxY);
    void drawWorld();
    void drawScreen(bool drawCenterPoint, float windowWidth, float windowMaxWidth, float windowHeight, float windowMaxHeight);
    void calcWorldCoords(float scaleX, float scaleY, float minX, float minY);
    void moveCenter(int newCenterX, int newCenterY);

    void toString(char *input);
    void fromString(char*output);

    int screenLeft;
    int screenTop;

    int screenLengthX;
    int screenLengthY;

    float worldLeft;
    float worldTop;

    float worldLengthX;
    float worldLengthY;
};
