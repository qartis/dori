#include <stddef.h>
#include <string>
#include <sstream>
#include <FL/Fl.H>
#include <FL/Fl_Tree.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>
#include <algorithm>
#include "siteobject.h"
#include "lineobject.h"

LineObject::LineObject() : worldWidth(0.0), worldLength(0.0) {
    type = LINE;
}

LineObject::~LineObject() { };

void LineObject::init(float worldX, float worldY, float newR, float newG, float newB) {
    worldOffsetX = worldX;
    worldOffsetY = worldY;

    r = newR;
    g = newG;
    b = newB;
}

void LineObject::drawWorld() {
    /*
    glPushMatrix();
    glColor3f(r / 255.0, g / 255.0, b / 255.0);

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

void LineObject::drawScreen(bool drawCenterPoint, float cellsPerMeter, float pixelsPerCell, float worldPanX, float worldPanY) {
    // Subtract out the world pan amount, then convert to screen coordinates
    // this gives us the offset (distance from the center of the site) in pixels
    // while taking panning into account

    float screenOffsetX = SiteObject::worldToScreen(worldOffsetX, pixelsPerCell, cellsPerMeter);
    float screenOffsetY = SiteObject::worldToScreen(worldOffsetY, pixelsPerCell, cellsPerMeter);

    float siteCenterScreenX = SiteObject::worldToScreen(siteCenterX - worldPanX, pixelsPerCell, cellsPerMeter);
    float siteCenterScreenY = SiteObject::worldToScreen(siteCenterY - worldPanY, pixelsPerCell, cellsPerMeter);

    float lineOriginX = siteCenterScreenX + screenOffsetX;
    float lineOriginY = siteCenterScreenY - screenOffsetY;

    float lineWidth = SiteObject::worldToScreen(worldWidth, pixelsPerCell, cellsPerMeter);
    float lineLength = SiteObject::worldToScreen(worldLength, pixelsPerCell, cellsPerMeter);

    // if the object is selected, draw the outline first
    if(selected) {
        int outline = 4;
        fl_line_style(FL_SOLID, 5 + outline);
        fl_color(255, 0, 0);
        fl_begin_loop();
        fl_vertex(lineOriginX, lineOriginY);
        fl_vertex(lineOriginX + lineWidth, lineOriginY - lineLength);
        fl_end_loop();
    }

    fl_line_style(FL_SOLID, 5);
    fl_color(r, g, b);
    fl_begin_loop();
    fl_vertex(lineOriginX, lineOriginY);
    fl_vertex(lineOriginX + lineWidth, lineOriginY - lineLength);
    fl_end_loop();

    if(drawCenterPoint) {
        fl_color(255, 0, 0);
        int centX = siteCenterScreenX + (cellsPerMeter * getWorldOffsetCenterX()) * pixelsPerCell;
        int centY = siteCenterScreenY - (cellsPerMeter * getWorldOffsetCenterY()) * pixelsPerCell;
        fl_circle(centX, centY, 1);
    }
}

void LineObject::updateWorldOffsetCenterX(float offsetCenterX) {
    if(locked) return;
    worldOffsetX += offsetCenterX;
}

void LineObject::updateWorldOffsetCenterY(float offsetCenterY) {
    if(locked) return;
    worldOffsetY += offsetCenterY;
}

float LineObject::getWorldOffsetCenterX() {
    return worldOffsetX + (worldWidth / 2.0);
}

float LineObject::getWorldOffsetCenterY() {
    return worldOffsetY + (worldLength / 2.0);
}

void LineObject::updateSize(float worldDifferenceX, float worldDifferenceY) {
    if(locked) return;
    worldWidth  = worldDifferenceX;
    worldLength = worldDifferenceY;
}

void LineObject::confirm() {
}

void LineObject::cancel() {
}

std::string LineObject::toString() {
    std::ostringstream ss;

    ss << worldOffsetX << " " << worldOffsetY << " ";
    ss << worldWidth   << " " << worldLength  << " ";
    ss << elevation    << " " << worldHeight  << " ";

    ss << r << " "  << g   << " " << b << " ";
    ss << (int)type << " " << (int)locked;

    return ss.str();
}

bool LineObject::fromString(char* input) {
    std::string temp = input;
    size_t count = std::count(temp.begin(), temp.end(), ' ');
    if(count < 10) {
        fprintf(stderr, "LineObject::fromString(): Invalid record: %s\n", input);
        return false;
    }

    sscanf(input,
           "%f %f "
           "%f %f "
           "%f %f "
           "%u %u %u "
           "%d "
           "%d ",
           &worldOffsetX, &worldOffsetY,
           &worldWidth, &worldLength,
           &elevation, &worldHeight,
           &r, &g, &b,
           (int*)&type,
           (int*)&locked);

    return true;
}
