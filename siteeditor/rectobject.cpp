#include <stddef.h>
#include "siteobject.h"
#include "rectobject.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>

void RectObject::scaleWorldCoords(float midX, float minY, float midY, float maxY) {
    worldLeft -= midX;
    worldTop = maxY - worldTop + minY - midY;
}

void RectObject::drawWorld() {
    glPushMatrix();
    glColor3f(r / 255.0, g / 255.0, b / 255.0);

    //fprintf(stderr, "world coords: (%f, %f), (%f, %f), %f\n", worldLeft, worldTop, worldLeft + worldWidth, worldTop + worldLength, worldHeight);

    //fprintf(stderr, "drawing rect: %f %f %f %f\n", worldLeft, worldTop, worldLeft + worldWidth, worldTop + worldLength);
    //bottom
    glTranslatef(worldLeft, elevation - 0.15, worldTop);

    glBegin(GL_POLYGON);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(worldWidth, 0.0, 0.0);
        glVertex3f(worldWidth, 0.0, worldLength);
        glVertex3f(0.0, 0.0, worldLength);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3f(0.0, worldHeight, 0.0);
        glVertex3f(worldWidth, worldHeight, 0.0);
        glVertex3f(worldWidth, worldHeight, worldLength);
        glVertex3f(0.0, worldHeight, worldLength);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(worldWidth, 0.0, 0.0);
        glVertex3f(worldWidth, worldHeight, 0.0);
        glVertex3f(0.0, worldHeight, 0.0);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(0.0, worldHeight, 0.0);
        glVertex3f(0.0, worldHeight, worldLength);
        glVertex3f(0.0, 0.0, worldLength);
    glEnd();

    glBegin(GL_POLYGON);
        glVertex3f(0.0, 0.0, worldLength);
        glVertex3f(worldWidth, 0.0, worldLength);
        glVertex3f(worldWidth, worldHeight, worldLength);
        glVertex3f(0.0, worldHeight, worldLength);
    glEnd();




    glPopMatrix();

    /*
    //front
    glBegin(GL_POLYGON);
    glVertex3f(worldLeft, 0.0, worldTop + worldLength);
    glVertex3f(worldLeft + worldWidth, 0.0, worldTop + worldLength);
    glVertex3f(worldLeft + worldWidth, worldHeight, worldTop + worldLength);
    glVertex3f(worldLeft, worldHeight, worldTop + worldLength);
    glEnd();

    //top
    glBegin(GL_POLYGON);
    glVertex3f(worldLeft, worldHeight, worldTop);
    glVertex3f(worldLeft + worldWidth, worldHeight, worldTop);
    glVertex3f(worldLeft + worldWidth, worldHeight, worldTop + worldLength);
    glVertex3f(worldLeft, worldHeight, worldTop + worldLength);
    glEnd();
    */
}

void RectObject::drawScreen(bool drawCenterPoint, float windowWidth, float windowMaxWidth, float windowHeight, float windowMaxHeight) {
    float scaledScreenLeft, scaledScreenTop, scaledScreenWidth, scaledScreenHeight;

    scaledScreenLeft = screenLeft * (windowWidth / windowMaxWidth);
    scaledScreenTop = screenTop * (windowHeight / windowMaxHeight);

    scaledScreenWidth = screenWidth * (windowWidth / windowMaxWidth);
    scaledScreenHeight = screenHeight * (windowHeight / windowMaxHeight);

    fl_color(r, g, b);
    fl_begin_loop();
    fl_vertex(scaledScreenLeft, scaledScreenTop);
    fl_vertex(scaledScreenLeft + scaledScreenWidth, scaledScreenTop);
    fl_vertex(scaledScreenLeft + scaledScreenWidth, scaledScreenTop + scaledScreenHeight);
    fl_vertex(scaledScreenLeft, scaledScreenTop + scaledScreenHeight);
    fl_vertex(scaledScreenLeft, scaledScreenTop);
    fl_end_loop();

    if(drawCenterPoint) {
        fl_color(255, 0, 0);
        fl_circle(scaledScreenCenterX(windowWidth, windowMaxWidth),
                  scaledScreenCenterY(windowHeight, windowMaxHeight),
                  1);
    }
}

void RectObject::moveCenter(int newCenterX, int newCenterY) {
    screenLeft = newCenterX - (screenWidth / 2);
    screenTop =  newCenterY - (screenHeight / 2);
    screenCenterX = newCenterX;
    screenCenterY = newCenterY;
}

void RectObject::calcWorldCoords(float scaleX, float scaleY, float minX, float minY) {
    worldLeft = ((screenLeft / scaleX) + minX) / 100.0;
    worldTop =  ((screenTop / scaleY) + minY) / 100.0;
    worldHeight = 1;

    worldWidth = (screenWidth / scaleX) / 100.0;
    worldLength =  (screenHeight / scaleY) / 100.0;
}

void RectObject::toString(char* output) {
    if(output == NULL) {
        fprintf(stderr, "output is NULL in serialize()\n");
        return;
    }

    sprintf(output, "%d %d %d %d "
           "%d %d "
           "%f %f %f %f "
           "%f %f "
           "%u %u %u "
           "%d ",
           screenLeft, screenTop, screenWidth, screenHeight,
           screenCenterX, screenCenterY,
           worldLeft, worldTop, worldWidth, worldLength,
           elevation, worldHeight,
           r, g, b,
           (int)type);
}

void RectObject::fromString(char* input) {
    sscanf(input, "%d %d %d %d "
           "%d %d "
           "%f %f %f %f "
           "%f %f "
           "%u %u %u "
           "%d ",
           &screenLeft, &screenTop, &screenWidth, &screenHeight,
           &screenCenterX, &screenCenterY,
           &worldLeft, &worldTop, &worldWidth, &worldLength,
           &elevation, &worldHeight,
           &r, &g, &b,
           (int*)&type);
}
