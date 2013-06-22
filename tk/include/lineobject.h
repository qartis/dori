class LineObject : public SiteObject {
public:
    LineObject();

    virtual ~LineObject();

    void drawWorld();
    void drawScreen(bool drawCenterPoint, float cellsPerMeter, int pixelsPerCell, int doriScreenX, int doriScreenY);
    float getWorldOffsetCenterX();
    float getWorldOffsetCenterY();

    void setWorldOffsetCenterX(float offsetCenterX);
    void setWorldOffsetCenterY(float offsetCenterY);

    void toString(char* input);
    void fromString(char* output);

    float worldWidth;
    float worldLength;
};
