#include <stddef.h>
#include "siteobject.h"
#include "lineobject.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>

void LineObject::scaleWorldCoords(float midX, float minY, float midY, float maxY) {
    worldLeft -= midX;
    worldTop = maxY - worldTop + minY - midY;
}

void LineObject::drawWorld() {
    glColor3f(r / 255.0, g / 255.0, b / 255.0);
    //
    // include the offsets
    glBegin(GL_POLYGON);
    glVertex3f(worldLeft, 0.0, worldTop);
    glVertex3f(worldLeft + worldLengthX, 0.0, worldTop);
    glVertex3f(worldLeft + worldLengthX, 0.0, worldTop + worldLengthY);
    glVertex3f(worldLeft, 0.0, worldTop + worldLengthY);
    glVertex3f(worldLeft, 0.0, worldTop);
    glEnd();

}

void LineObject::drawScreen(bool drawCenterPoint, float windowWidth, float windowMaxWidth, float windowHeight, float windowMaxHeight) {
    float scaledScreenLeft, scaledScreenTop, scaledScreenLengthX, scaledScreenLengthY;

    scaledScreenLeft = screenLeft * (windowWidth / windowMaxWidth);
    scaledScreenTop = screenTop * (windowHeight / windowMaxHeight);

    scaledScreenLengthX = screenLengthX * (windowWidth / windowMaxWidth);
    scaledScreenLengthY = screenLengthY * (windowHeight / windowMaxHeight);

    fl_color(r, g, b);
    fl_line(scaledScreenLeft, scaledScreenTop, scaledScreenLeft + scaledScreenLengthX, scaledScreenTop + scaledScreenLengthY);

    if(drawCenterPoint) {
        fl_color(255, 0, 0);
        fl_circle(scaledScreenCenterX(windowWidth, windowMaxWidth),
                  scaledScreenCenterY(windowHeight, windowMaxHeight),
                  1);
    }

}

void LineObject::moveCenter(int newCenterX, int newCenterY) {

    screenLeft = newCenterX - (screenLengthX / 2);
    screenTop =  newCenterY - (screenLengthY / 2);

    screenCenterX = newCenterX;
    screenCenterY = newCenterY;
}

void LineObject::calcWorldCoords(float scaleX, float scaleY, float minX, float minY) {
    worldLeft = (screenLeft / scaleX) + minX;
    worldTop =  (screenTop / scaleY) + minY;

    worldLengthX = (screenLengthX / scaleX);
    worldLengthY =  (screenLengthY / scaleY);
}

void LineObject::toString(char* output) {
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
            screenLeft, screenTop, screenLengthX, screenLengthY,
            screenCenterX, screenCenterY,
            worldLeft, worldTop, worldLengthX, worldLengthY,
            elevation, worldHeight,
            r, g, b,
            type);
}

void LineObject::fromString(char* input) {
    sscanf(input, "%d %d %d %d "
           "%d %d "
           "%f %f %f %f "
           "%f %f "
           "%u %u %u "
           "%d ",
           &screenLeft, &screenTop, &screenLengthX, &screenLengthY,
           &screenCenterX, &screenCenterY,
           &worldLeft, &worldTop, &worldLengthX, &worldLengthY,
           &elevation, &worldHeight,
           &r, &g, &b,
           (int*)&type);
}
