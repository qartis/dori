#include "siteobject.h"

SiteObject::SiteObject() {
    r = backupR = 0;
    g = backupG = 0;
    b = backupB = 0;

    // This is temporary until the database is plugged in
    siteCenterX = SITE_METER_EXTENTS;
    siteCenterY = SITE_METER_EXTENTS;

    selected = false;
}

// # cells = pixels * (cells / pixel)
// worldCoords =  # cells * metersPerCell
float SiteObject::screenToWorld(float screenVal, float cellsPerPixel, float metersPerCell) {
    return screenVal * cellsPerPixel * metersPerCell;
}

// # cells = meters * (cells / meter)
// screenCoords =  # cells * pixelsPerCell
float SiteObject::worldToScreen(float worldVal, float pixelsPerCell, float cellsPerMeter) {
    return worldVal * cellsPerMeter * pixelsPerCell;
}

void SiteObject::select() {
    if(!selected) {
        selected = true;
    }
}

void SiteObject::unselect() {
    if(selected) {
        selected = false;
    }
}

void SiteObject::setRGB(unsigned red, unsigned green, unsigned blue) {
    r = red;
    g = green;
    b = blue;
}



