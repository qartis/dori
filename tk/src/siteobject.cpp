#include <string>
#include <sstream>
#include <FL/Fl_Tree.H>
#include "siteobject.h"

SiteObject::SiteObject() {
    r = backupR = 0;
    g = backupG = 0;
    b = backupB = 0;
    
    worldHeight = 0.0;
    elevation = 0.0;
    
    locked = false;

    // This is temporary until the database is plugged in
    siteCenterX = SITE_METER_EXTENTS;
    siteCenterY = SITE_METER_EXTENTS;

    selected = false;

    siteid = -1; 
    recordType = "";
    treeItem = NULL;
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

void SiteObject::setLocked(bool lock) {
    locked = lock;
}

std::string SiteObject::buildTreeString() {
    std::ostringstream ssTree;

    // special siteID for annotations
    if(siteid == -1) {
        ssTree << "Annotations";
    }
    else {
        ssTree << "Site " << siteid;
    }

    ssTree << "/";

    // Check if this site object has a record type from TK
    if(recordType.length() > 0) {
        ssTree << recordType;
    }
    else {
        switch(type) {
        case LINE:
            ssTree << "Lines";
            break;
        case RECT:
            ssTree << "Rectanges";
            break;
        case CIRCLE:
            ssTree << "Circles";
            break;
        case POLY:
            ssTree << "Polygons";
            break;
        default:
            ssTree << "Undefined Object";
        }
    }

    ssTree << "/" << rowid;

    return ssTree.str();
}

