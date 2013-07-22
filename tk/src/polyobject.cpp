#include <stddef.h>
#include <vector>
#include <string>
#include <sstream>
#include "siteobject.h"
#include "polyobject.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>

PolyObject::PolyObject() {
    type = POLY;
    nextPoint = NULL;
    recalculateCenterPoint = true;
}

PolyObject::~PolyObject() {
    if(nextPoint) {
        delete nextPoint;
        nextPoint = NULL;
    }
}

void PolyObject::init(float worldX, float worldY, float newR, float newG, float newB) {
    worldOffsetX = worldX;
    worldOffsetY = worldY;

    // Add the first point explicitly to the poly object as well
    // so that its list of points includes the 'worldOffset' point
    // which is the (world) distance from the center of the site
    // to the 'origin' of the site object
    addPoint(0.0, 0.0);

    r = newR;
    g = newG;
    b = newB;
}

void PolyObject::drawWorld() {
}

void PolyObject::drawScreen(bool drawCenterPoint, float cellsPerMeter, float pixelsPerCell, float worldPanX, float worldPanY) {
    float screenOffsetX = SiteObject::worldToScreen(worldOffsetX, pixelsPerCell, cellsPerMeter);
    float screenOffsetY = SiteObject::worldToScreen(worldOffsetY, pixelsPerCell, cellsPerMeter);


    float siteCenterScreenX = SiteObject::worldToScreen(siteCenterX - worldPanX, pixelsPerCell, cellsPerMeter);
    float siteCenterScreenY = SiteObject::worldToScreen(siteCenterY - worldPanY, pixelsPerCell, cellsPerMeter);

    float polyOriginX = siteCenterScreenX + screenOffsetX;
    float polyOriginY = siteCenterScreenY - screenOffsetY;

    // if the object is selected, draw the outline first
    if(selected) {
        int outline = 4;
        fl_line_style(FL_SOLID, 5 + outline);
        fl_color(255, 0, 0);
        drawPolyLine(polyOriginX, polyOriginY, pixelsPerCell, cellsPerMeter);
    }

    fl_line_style(FL_SOLID, 5);
    fl_color(r, g, b);

    drawPolyLine(polyOriginX, polyOriginY, pixelsPerCell, cellsPerMeter);

    if(drawCenterPoint) {
        fl_color(255, 0, 0);

        if(recalculateCenterPoint) {
            calculateCenterPoint();
            recalculateCenterPoint = false;
        }

        float centerPointScreenOffsetX = SiteObject::worldToScreen(centerPointWorldOffsetX, pixelsPerCell, cellsPerMeter);
        float centerPointScreenOffsetY = SiteObject::worldToScreen(centerPointWorldOffsetY, pixelsPerCell, cellsPerMeter);

        fl_circle(siteCenterScreenX + centerPointScreenOffsetX, siteCenterScreenY - centerPointScreenOffsetY, 1);
    }
}

void PolyObject::drawPolyLine(float polyOriginX, float polyOriginY, float pixelsPerCell, float cellsPerMeter) {
    fl_begin_loop();

    for(unsigned i = 0; i < points.size(); i++) {
        float pointOffsetX = SiteObject::worldToScreen(points[i].x, pixelsPerCell, cellsPerMeter);
        float pointOffsetY = SiteObject::worldToScreen(points[i].y, pixelsPerCell, cellsPerMeter);

        float pointX = polyOriginX + pointOffsetX;
        float pointY = polyOriginY - pointOffsetY;

        fl_vertex(pointX, pointY);
    }

    if(nextPoint) {
        float pointOffsetX = SiteObject::worldToScreen(nextPoint->x, pixelsPerCell, cellsPerMeter);
        float pointOffsetY = SiteObject::worldToScreen(nextPoint->y, pixelsPerCell, cellsPerMeter);

        float pointX = polyOriginX + pointOffsetX;
        float pointY = polyOriginY - pointOffsetY;

        fl_vertex(pointX, pointY);
    }

    fl_end_loop();


}

void PolyObject::calculateCenterPoint() {

    if(points.size() == 1) {
        centerPointWorldOffsetX = worldOffsetX;
        centerPointWorldOffsetY = worldOffsetY;
        return;
    }

    if(points.size() == 2) {
        centerPointWorldOffsetX = worldOffsetX + (points[0].x + points[1].x) / 2.0;
        centerPointWorldOffsetY = worldOffsetY + (points[0].y + points[1].y) / 2.0;
        return;
    }

    float centerX = 0.0;
    float centerY = 0.0;

    float x0 = 0.0;
    float y0 = 0.0;

    float x1 = 0.0;
    float y1 = 0.0;

    float area = 0.0;

    /***** Calculation for centroid of a (non-self-intersecting) polygon ****/
    for(unsigned i = 0; i < points.size() - 1; i++) {
        x0 = points[i].x;
        y0 = points[i].y;

        x1 = points[i+1].x;
        y1 = points[i+1].y;

        area += (x0 * y1) - (x1 * y0);
    }

    area += (points[points.size()-1].x * points[0].y) - (points[0].x * points[points.size()-1].y);
    area /= 2.0;

    float scalingFactor = (1 / (6 * area));

    for(unsigned i = 0; i < points.size()-1; i++) {
        x0 = points[i].x;
        y0 = points[i].y;

        x1 = points[i+1].x;
        y1 = points[i+1].y;

        centerX += (x0 + x1) * ((x0 * y1) - (x1 * y0));
        centerY += (y0 + y1) * ((x0 * y1) - (x1 * y0));
    }

    centerX += (points[points.size()-1].x + points[0].x) * ((points[points.size()-1].x * points[0].y) - (points[0].x * points[points.size()-1].y));
    centerX *= scalingFactor;

    centerY += (points[points.size()-1].y + points[0].y) * ((points[points.size()-1].x * points[0].y) - (points[0].x * points[points.size()-1].y));
    centerY *= scalingFactor;

    // Add the world offset to the center point
    // because all points are using worldOffset as origin
    centerPointWorldOffsetX = worldOffsetX + centerX;
    centerPointWorldOffsetY = worldOffsetY + centerY;
}


void PolyObject::updateWorldOffsetCenterX(float offsetCenterX) {
    worldOffsetX += offsetCenterX;
    recalculateCenterPoint = true;
}

void PolyObject::updateWorldOffsetCenterY(float offsetCenterY) {
    worldOffsetY += offsetCenterY;
    recalculateCenterPoint = true;
}

float PolyObject::getWorldOffsetCenterX() {
    return centerPointWorldOffsetX;
}

float PolyObject::getWorldOffsetCenterY() {
    return centerPointWorldOffsetY;
}

void PolyObject::addPoint(float x, float y) {
    Point p(x, y);
    points.push_back(p);
    recalculateCenterPoint = true;
}

void PolyObject::setNextPoint(float x, float y) {
    if(nextPoint == NULL) {
        nextPoint = new Point;
    }

    nextPoint->x = x;
    nextPoint->y = y;

    recalculateCenterPoint = true;
}

// Resize the object based on world coordinates
void PolyObject::updateSize(float worldDifferenceX, float worldDifferenceY) {
    float x, y;

    if(points.size() > 0) {
        Point& lastPoint = points.back();
        x = lastPoint.x + worldDifferenceX;
        y = lastPoint.y + worldDifferenceY;
    }
    else {
        x = worldOffsetX + worldDifferenceX;
        y = worldOffsetY + worldDifferenceY;
    }

    setNextPoint(x, y);
}

void PolyObject::confirm() {
    if(nextPoint) {
        addPoint(nextPoint->x, nextPoint->y);
        delete nextPoint;
        nextPoint = NULL;
    }
}

void PolyObject::cancel() {
    if(nextPoint) {
        delete nextPoint;
        nextPoint = NULL;
    }
    recalculateCenterPoint = true;
}

std::string PolyObject::toString() {
    std::ostringstream ss;

    ss << worldOffsetX << " " << worldOffsetY << " " << points.size();

    for(unsigned i = 0; i < points.size(); i++) {
        ss << " " << points[i].x    << " " << points[i].y;
    }

    ss << " " << elevation << " "   << worldHeight;
    ss << " " << r << " "  << g  << " "  << b;
    ss << " " << type;

    return ss.str();
}

void PolyObject::fromString(char* input) {
    unsigned total_points = 0;
    std::istringstream ss(input);

    ss >> worldOffsetX >> worldOffsetY;
    ss >> total_points;

    for(unsigned i = 0; i < total_points; i++) {
        float x = 0.0;
        float y = 0.0;
        ss >> x >> y;
        Point p(x, y);
        points.push_back(p);
    }

    ss >> elevation >> worldHeight;
    ss >> r >> g >> b;

    //TODO add istream >> support to the enum type
    int typeTemp = 0;
    ss >> typeTemp;
    type = (SiteObjectType)typeTemp;
}

int PolyObject::numPoints() {
    return points.size();
}

