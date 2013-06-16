class RectObject : public SiteObject {
public:
    RectObject();
    ~RectObject();

    void drawWorld();
    void drawScreen(bool drawCenterPoint, int windowWidth, int windowLength, float siteMeterExtents);
    float getWorldOffsetCenterX();
    float getWorldOffsetCenterY();

    void setWorldOffsetCenterX(float offsetCenterX);
    void setWorldOffsetCenterY(float offsetCenterY);

    void toString(char* input);
    void fromString(char* output);

    float worldOffsetX;
    float worldOffsetY;

    float worldWidth;
    float worldLength;
};
