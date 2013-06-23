class CircleObject : public SiteObject {
public:
    CircleObject();
    ~CircleObject();

    void drawWorld();
    void drawScreen(bool drawCenterPoint, float cellsPerMeter, float pixelsPerCell, int doriScreenX, int doriScreenY);
    float getWorldOffsetCenterX();
    float getWorldOffsetCenterY();

    void setWorldOffsetCenterX(float offsetCenterX);
    void setWorldOffsetCenterY(float offsetCenterY);

    void toString(char* input);
    void fromString(char* output);

    float worldRadius;
};
