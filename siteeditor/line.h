struct Line : public Geometry {
    Line() :
        screenLeft(0), screenTop(0),
        screenLengthX(0), screenLengthY(0),
        worldLeft(0.0), worldTop(0.0),
        worldLengthX(0.0), worldLengthY(0.0)
    {
        type = LINE;
    }

    int screenLeft;
    int screenTop;

    int screenLengthX;
    int screenLengthY;

    float worldLeft;
    float worldTop;

    float worldLengthX;
    float worldLengthY;
};
