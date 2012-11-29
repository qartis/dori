#ifndef __OBJMODEL__
#define __OBJMODEL__

#include <vector>
#include "vectors.h"
#include <cstdio>

class ObjModel {

public:
    std::vector<Vector3f> verts;
    std::vector<Vector4i> faces;

    ObjModel() {
    };

    ~ObjModel() {
    };

    void load(const char * filename);
    void draw(float x, float y, float z);

private:

    FILE *fp;
};

#endif
