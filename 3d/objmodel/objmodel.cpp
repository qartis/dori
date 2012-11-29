#include "objmodel.h"
#include <cstdio>
#include <GL/glu.h>

#define BUFF_SIZE 64

void ObjModel::load(const char * filename) {
    fp = fopen(filename, "rt");

    char line[BUFF_SIZE];
    char type;
    float a = 0.0, b = 0.0, c = 0.0, d = 0.0;
    while(fgets(line, BUFF_SIZE, fp) != NULL) {
        int num_read = sscanf(line, "%c %f %f %f %f", &type, &a, &b, &c, &d);
        // we'll only grab lines that have either 4 or 5 items
        // including the character that defines what the line describes
        if(num_read >= 4 && num_read <= 5) {
            if(type == 'v') { // processing a vertex
                Vector3f vec = {a, b, c};
                verts.push_back(vec);
            }
            else if(type == 'f') { // processing a face
                Vector4i face;
                face.count = num_read - 1;
                face.a = (int)a - 1;
                face.b = (int)b - 1;
                face.c = (int)c - 1;
                face.d = (int)d - 1;
                faces.push_back(face);

//                printf("%d %d %d %d\n", face.a, face.b, face.c, face.d);
            }
        }
    }
}

void ObjModel::draw(float x, float y, float z) {
    float finalX = 0.0, finalY = 0.0, finalZ = 0.0;
    for(int i = 0; i < faces.size(); i++) {
        glBegin(GL_POLYGON);

        glVertex3f(+ verts[faces[i].a].x,
                y + verts[faces[i].a].y,
                z + verts[faces[i].a].z);

        glVertex3f(x + verts[faces[i].b].x,
                   y + verts[faces[i].b].y,
                   z + verts[faces[i].b].z);

        glVertex3f(x + verts[faces[i].c].x,
                   y + verts[faces[i].c].y,
                   z + verts[faces[i].c].z);

        if(faces[i].count == 4) {
            glVertex3f(x + verts[faces[i].d].x,
                       y + verts[faces[i].d].y,
                       z + verts[faces[i].d].z);
        }
        glEnd();
    }
}

