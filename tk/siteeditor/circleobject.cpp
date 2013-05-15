#include <stddef.h>
#include "siteobject.h" 
#include "circleobject.h" 
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>

void CircleObject::scaleWorldCoords(float midX, float minY, float midY, float maxY) {
}

void CircleObject::drawWorld() {
}

void CircleObject::drawScreen(bool drawCenterPoint, float windowWidth, float windowMaxWidth, float windowHeight, float windowMaxHeight) {
    fl_color(r, g, b);

    int scaledCenterX, scaledCenterY;

    scaledCenterX = scaledScreenCenterX(windowWidth, windowMaxWidth);
    scaledCenterY = scaledScreenCenterY(windowHeight, windowMaxHeight);

    int scaledRadiusUsingWidth = screenRadius * (windowWidth / windowMaxWidth);
    int scaledRadiusUsingHeight = screenRadius * (windowHeight / windowMaxHeight);

    int scaledRadius = scaledRadiusUsingWidth < scaledRadiusUsingHeight ? scaledRadiusUsingWidth : scaledRadiusUsingHeight;

    if(scaledRadius < 2) {
        scaledRadius = 2;
    }

    if(strcmp(special, "filled") == 0) {
        fl_pie(scaledCenterX + scaledRadius, scaledCenterY + scaledRadius, scaledRadius, scaledRadius, 0, 360);
    }
    else {
        fl_circle(scaledCenterX, scaledCenterY, scaledRadius);
    }

    if(drawCenterPoint) {
        fl_color(255, 0, 0);
        fl_circle(scaledCenterX, scaledCenterY, 1);
    }
}

void CircleObject::moveCenter(int newCenterX, int newCenterY) {
    screenCenterX = newCenterX;
    screenCenterY = newCenterY;
}

void CircleObject::calcWorldCoords(float scaleX, float scaleY, float minX, float minY) {
    (void)scaleX;
    (void)scaleY;
    (void)minX;
    (void)minY;
}

void CircleObject::toString(char* output) {
    if(output == NULL) {
        fprintf(stderr, "output is NULL in serialize()\n");
        return;
    }

    sprintf(output, "%d "
            "%d %d "
            "%f %f %f "
            "%f %f "
            "%u %u %u "
            "%d ",
            screenRadius,
            screenCenterX, screenCenterY,
            worldCenterX, worldCenterY, worldRadius,
            elevation, worldHeight,
            r, g, b,
            (int)type);
}

void CircleObject::fromString(char* input) {
    sscanf(input, "%d "
            "%d %d "
            "%f %f %f "
            "%f %f "
            "%u %u %u "
            "%d ",
            &screenRadius,
            &screenCenterX, &screenCenterY,
            &worldCenterX, &worldCenterY, &worldRadius,
            &elevation, &worldHeight,
            &r, &g, &b,
            (int*)&type);
}
