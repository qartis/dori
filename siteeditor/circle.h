struct Circle : public Geometry {
    //screen center is in geometry.h

    Circle() :
        screenRadius(0),
        worldCenterX(0.0), worldCenterY(0.0),
        worldRadius(0.0)
    {
        type = CIRCLE;
    }

    int screenRadius;

    float worldCenterX;
    float worldCenterY;
    float worldRadius;
};
