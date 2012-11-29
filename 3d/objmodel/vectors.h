#ifndef __VECTORS__
#define __VECTORS__

struct Vector3f {
    float x;
    float y;
    float z;
};

struct Vector4i {
    int count;
    int a;
    int b;
    int c;
    int d;

    Vector4i()
    {
        count = 0;
    }
};

#endif
