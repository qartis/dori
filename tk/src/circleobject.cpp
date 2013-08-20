#include <stddef.h>
#include <string>
#include <sstream>
#include <FL/Fl.H>
#include <FL/Fl_Tree.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>
#include <algorithm>
#include "siteobject.h"
#include "circleobject.h"

CircleObject::CircleObject() : worldRadius(0.0) {
    type = CIRCLE;
}

CircleObject::~CircleObject() { };

void CircleObject::init(float worldX, float worldY, float newR, float newG, float newB) {
    worldOffsetX = worldX;
    worldOffsetY = worldY;

    r = newR;
    g = newG;
    b = newB;
}

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

void CircleObject::updateWorldOffsetCenterX(float offsetCenterX) {
    if(locked) return;
    worldOffsetX += offsetCenterX;
}

void CircleObject::updateWorldOffsetCenterY(float offsetCenterY) {
    if(locked) return;
    worldOffsetY += offsetCenterY;
}

float CircleObject::getWorldOffsetCenterX() {
    return worldOffsetX;
}

float CircleObject::getWorldOffsetCenterY() {
    return worldOffsetY;
}


// Resize the object based on world coordinates
void CircleObject::updateSize(float worldDifferenceX, float worldDifferenceY) {
    if(locked) return;

    // set the radius to the max mouse difference
    worldRadius = std::max(worldDifferenceX, worldDifferenceY);
}

void CircleObject::confirm() {
    // nothing to do on confirmation
}

void CircleObject::cancel() {
}

std::string CircleObject::toString() {
    std::ostringstream ss;

    ss << worldOffsetX << " " << worldOffsetY << " ";
    ss << worldRadius  << " ";
    ss << elevation    << " " << worldHeight  << " ";

    ss << r << " " << g << " " << b << " ";
    ss << (int)type << (int)locked;

    return ss.str();
}

bool CircleObject::fromString(char* input) {
    std::string temp = input;
    size_t count = std::count(temp.begin(), temp.end(), ' ');
    if(count < 8) {
        fprintf(stderr, "CircleObject::fromString(): Invalid record: %s\n", input);
        return false;
    }

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
           (int*)&locked);

    return true;
}
