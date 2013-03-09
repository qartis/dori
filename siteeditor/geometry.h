struct Geometry {
    Geometry() :
        id(0), screenCenterX(0), screenCenterY(0),
        elevation(0.0), worldHeight(0.0),
        r(0), g(0), b(0), type(GEOMETRY)
    {
    }

    int id;

    int screenCenterX;
    int screenCenterY;

    float elevation;
    float worldHeight;

    unsigned r;
    unsigned g;
    unsigned b;

    enum geomType {
        GEOMETRY,
        LINE,
        RECT,
        CIRCLE,
    };

    geomType type;
};
