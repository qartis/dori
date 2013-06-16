#include "siteobject.h"

SiteObject::SiteObject() {
    r = backupR = 0;
    g = backupG = 0;
    b = backupB = 0;

    selected = false;
}

float SiteObject::screenToWorld(int screenVal, float meterExtents, int pixelExtents) {
    return (meterExtents / pixelExtents)  * screenVal;
}

int SiteObject::worldToScreen(int worldVal, float meterExtents, int pixelExtents) {
    return (pixelExtents / meterExtents)  * worldVal;
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



