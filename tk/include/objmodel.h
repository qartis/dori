#include <vector>
#include "vectors.h"
#include <string.h>
#include <cstdio>

class ObjModel {

public:
    std::vector<Vector3f> verts;
    std::vector<Vector4i> faces;

    ObjModel() : xPos(0), yPos(0), zPos(0), xRot(0), yRot(0), zRot(0) {
        memset(filename, 0, sizeof(filename));
    }

    ~ObjModel() {
    }

    void load(const char * file);
    void draw(bool newMatrix = true);

    const char* getFilename() {
        return filename;
    }

    void setPos(float x, float y, float z) {
        xPos = x;
        yPos = y;
        zPos = z;
    }

    void setRot(float x, float y, float z) {
        xRot = x;
        yRot = y;
        zRot = z;
    }

    float xPos;
    float yPos;
    float zPos;

    float xRot;
    float yRot;
    float zRot;

private:

        char filename[256];

    FILE *fp;
};
