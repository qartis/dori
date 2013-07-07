#include <stddef.h>
#include "siteobject.h"
#include "lineobject.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>

LineObject::LineObject() : worldWidth(0.0), worldLength(0.0) {
    type = LINE;
}

LineObject::~LineObject() { };

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

void LineObject::setWorldOffsetCenterX(float offsetCenterX) {
    worldOffsetX = offsetCenterX - (worldWidth / 2.0);
}

void LineObject::setWorldOffsetCenterY(float offsetCenterY) {
    worldOffsetY = offsetCenterY - (worldLength / 2.0);
}

float LineObject::getWorldOffsetCenterX() {
    return worldOffsetX + (worldWidth / 2.0);
}

float LineObject::getWorldOffsetCenterY() {
    return worldOffsetY + (worldLength / 2.0);
}

void LineObject::toString(char* output) {
    if(output == NULL) {
        fprintf(stderr, "output is NULL in serialize()\n");
        return;
    }
    sprintf(output,
           "%f %f "
           "%f %f "
           "%f %f "
           "%u %u %u "
           "%d ",
           worldOffsetX, worldOffsetY,
           worldWidth, worldLength,
           elevation, worldHeight,
           r, g, b,
           (int)type);
}

void LineObject::fromString(char* input) {
    sscanf(input,
           "%f %f "
           "%f %f "
           "%f %f "
           "%u %u %u "
           "%d ",
           &worldOffsetX, &worldOffsetY,
           &worldWidth, &worldLength,
           &elevation, &worldHeight,
           &r, &g, &b,
           (int*)&type);
}
