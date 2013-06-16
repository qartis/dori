#include <stddef.h>
#include "siteobject.h"
#include "rectobject.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>

RectObject::RectObject() : worldOffsetX(0.0), worldOffsetY(0.0), worldWidth(0.0), worldLength(0.0) {
    type = RECT;
}

RectObject::~RectObject() { };

void RectObject::drawWorld() {
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

void RectObject::drawScreen(bool drawCenterPoint, int windowWidth, int windowLength, float siteMeterExtents) {
    // (pixels / meter extents for a site) * offset from DORI
    // (windowWidth / 100 meters) * offset from DORI in meters
    // eventually do 100 meters * zoomLevel
    int screenOffsetX = (float)(windowWidth / siteMeterExtents) * worldOffsetX;
    int screenOffsetY = (float)(windowLength / siteMeterExtents) * worldOffsetY;

    int doriScreenX = windowWidth / 2.0;
    int doriScreenY = windowLength / 2.0;

    int rectOriginX = doriScreenX + screenOffsetX;
    int rectOriginY = doriScreenY - screenOffsetY;

    int rectWidth   = SiteObject::worldToScreen(worldWidth, siteMeterExtents, windowWidth);
    int rectLength  = SiteObject::worldToScreen(worldLength, siteMeterExtents, windowLength);

    // if the object is selected, draw the outline first
    if(selected) {
        int outline = 4;
        fl_line_style(FL_SOLID, 5 + outline);
        fl_color(255, 0, 0);
        fl_begin_loop();
        fl_vertex(rectOriginX, rectOriginY);
        fl_vertex(rectOriginX + rectWidth, rectOriginY);
        fl_vertex(rectOriginX + rectWidth, rectOriginY - rectLength);
        fl_vertex(rectOriginX, rectOriginY - rectLength);
        fl_vertex(rectOriginX, rectOriginY);
        fl_end_loop();
    }

    fl_line_style(FL_SOLID, 5);
    fl_color(r, g, b);
    fl_begin_loop();
    fl_vertex(rectOriginX, rectOriginY);
    fl_vertex(rectOriginX + rectWidth, rectOriginY);
    fl_vertex(rectOriginX + rectWidth, rectOriginY - rectLength);
    fl_vertex(rectOriginX, rectOriginY - rectLength);
    fl_vertex(rectOriginX, rectOriginY);
    fl_end_loop();

    if(drawCenterPoint) {
        fl_color(255, 0, 0);
        int centX = doriScreenX + SiteObject::worldToScreen(getWorldOffsetCenterX(), siteMeterExtents, windowWidth);
        int centY = doriScreenY - SiteObject::worldToScreen(getWorldOffsetCenterY(), siteMeterExtents, windowLength);
        fl_circle(centX, centY, 1);
    }
}

void RectObject::setWorldOffsetCenterX(float offsetCenterX) {
    worldOffsetX = offsetCenterX - (worldWidth / 2.0);
}

void RectObject::setWorldOffsetCenterY(float offsetCenterY) {
    worldOffsetY = offsetCenterY - (worldLength / 2.0);
}

float RectObject::getWorldOffsetCenterX() {
    return worldOffsetX + (worldWidth / 2.0);
}

float RectObject::getWorldOffsetCenterY() {
    return worldOffsetY + (worldLength / 2.0);
}

void RectObject::toString(char* output) {
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

void RectObject::fromString(char* input) {
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
