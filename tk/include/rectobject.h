class RectObject : public SiteObject {
public:
    RectObject();
    ~RectObject();

    void init(float worldX, float worldY, float r, float g, float b);

    void drawWorld();
    void drawScreen(bool drawCenterPoint, float cellsPerMeter, float pixelsPerCell, float worldPanX, float worldPanY);
    float getWorldOffsetCenterX();
    float getWorldOffsetCenterY();

    void updateWorldOffsetCenterX(float offsetCenterX);
    void updateWorldOffsetCenterY(float offsetCenterY);

    void updateSize(float worldDifferenceX, float worldDifferenceY);
    void confirm();
    void cancel();

    std::string toString();
    void fromString(char* output);

    float worldWidth;
    float worldLength;
};
