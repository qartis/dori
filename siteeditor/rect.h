struct Rect : public Geometry {

    Rect() :
        screenLeft(0), screenTop(0),
        screenWidth(0), screenHeight(0),
        worldLeft(0.0), worldTop(0.0),
        worldWidth(0.0), worldLength(0.0)
    {
        type = RECT;
    }

    int screenLeft;
    int screenTop;

    int screenWidth;
    int screenHeight;

    float worldLeft;
    float worldTop;

    float worldWidth;
    float worldLength;
};
