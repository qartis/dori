#include <stddef.h>
#include "siteobject.h"
#include "circleobject.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>

CircleObject::CircleObject() : worldRadius(0.0) {
    type = CIRCLE;
}

CircleObject::~CircleObject() { };

void CircleObject::drawWorld() {
}

void CircleObject::drawScreen(bool drawCenterPoint, float cellsPerMeter, float pixelsPerCell, float worldPanX, float worldPanY) {    // # cells = (cells / meter) * (worldOffset in meters)
    // # cells * pixelsPerCell = position in pixels
    float screenOffsetX = SiteObject::worldToScreen(worldOffsetX, pixelsPerCell, cellsPerMeter);
    float screenOffsetY = SiteObject::worldToScreen(worldOffsetY, pixelsPerCell, cellsPerMeter);

    float siteCenterScreenX = SiteObject::worldToScreen(siteCenterX - worldPanX, pixelsPerCell, cellsPerMeter);
    float siteCenterScreenY = SiteObject::worldToScreen(siteCenterY - worldPanY, pixelsPerCell, cellsPerMeter);

    int circleOriginX = siteCenterScreenX + screenOffsetX;
    int circleOriginY = siteCenterScreenY - screenOffsetY;

    int circleRadius = (cellsPerMeter * worldRadius) * pixelsPerCell;

    // if the object is selected, draw the outline first
    if(selected) {
        int outline = 4;
        fl_line_style(FL_SOLID, 5 + outline);
        fl_color(255, 0, 0);
        fl_circle(circleOriginX, circleOriginY, circleRadius);
    }

    fl_line_style(FL_SOLID, 5);
    fl_color(r, g, b);
    fl_circle(circleOriginX, circleOriginY, circleRadius);

    if(drawCenterPoint) {
        fl_color(255, 0, 0);
        fl_circle(circleOriginX, circleOriginY, 1);
    }
}

void CircleObject::setWorldOffsetCenterX(float offsetCenterX) {
    worldOffsetX = offsetCenterX;
}

void CircleObject::setWorldOffsetCenterY(float offsetCenterY) {
    worldOffsetY = offsetCenterY;
}

float CircleObject::getWorldOffsetCenterX() {
    return worldOffsetX;
}

float CircleObject::getWorldOffsetCenterY() {
    return worldOffsetY;
}

void CircleObject::toString(char* output) {
    if(output == NULL) {
        fprintf(stderr, "output is NULL in serialize()\n");
        return;
    }
    sprintf(output,
           "%f %f "
           "%f "
           "%f %f "
           "%u %u %u "
           "%d ",
           worldOffsetX, worldOffsetY,
           worldRadius,
           elevation, worldHeight,
           r, g, b,
           (int)type);
}

void CircleObject::fromString(char* input) {
    sscanf(input,
           "%f %f "
           "%f "
           "%f %f "
           "%u %u %u "
           "%d ",
           &worldOffsetX, &worldOffsetY,
           &worldRadius,
           &elevation, &worldHeight,
           &r, &g, &b,
           (int*)&type);
}
