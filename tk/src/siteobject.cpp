#include "siteobject.h"

SiteObject::SiteObject() {
    r = backupR = 0;
    g = backupG = 0;
    b = backupB = 0;

    selected = false;
}

float SiteObject::screenToWorld(int screenVal, float meterExtents, int pixelExtents) {
    return ((float)meterExtents / (float)pixelExtents)  * (float)screenVal;
}

int SiteObject::worldToScreen(float worldVal, float meterExtents, int pixelExtents) {
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



