#include "radarwindow.h"
#include "../spawnablewindow.h"
#include <FL/fl_draw.H>
#include <math.h>
#include <limits.h>

#define MAX_ARCS 8

#define RADAR_TYPE "laser"

//So now I just need to have a pointer in RadarWindow to Table's Rows vector, and use that data to draw the points instead of the data[360]. Then use the pointCache to cache all the points by rowid.

RadarWindow::RadarWindow(int x, int y, int w, int h, const char *label) : SpawnableWindow(x, y, w, h, label) {
    insideAngle = 360.0;
    originX = w / 2.0;
    originY = h / 2.0;
    completeRedraw = false;
    scale = 2.0;
    radius = w / 16.0;
    curPointRowID = -1;
}

void RadarWindow::computeCoords(std::vector<Row>::iterator it, int &screenX, int &screenY) {
    if(!rowData) {
        return;
    }
    int rowid = atoi(it->cols[0]);

    DataPoint data;

    if(lookup(rowid, data)) {
        screenX = data.screenX;
        screenY = data.screenY;
    }
    else {
        float angle, distance, ratio;

        ratio = radius / scale;
        angle = atof(it->cols[2]);
        distance = atof(it->cols[3]);

        screenX = originX + (ratio * (distance * (cos((float)angle * (M_PI / 180.0)))));
        screenY = originY - (ratio * (distance * (sin((float)angle * (M_PI / 180.0)))));

        DataPoint newPoint;
        newPoint.screenX = screenX;
        newPoint.screenY = screenY;

        // cols[0] is rowid
        pointCache[rowid] = newPoint;
    }
}

int RadarWindow::rowidOfClosestPoint(int mouseX, int mouseY) {
    std::map<int, DataPoint>::iterator it;
    int minRowID, minSquare;
    int square;

    minSquare = INT_MAX;

    for(it = pointCache.begin(); it != pointCache.end(); it++) {
        square = ((mouseX - it->second.screenX) * (mouseX - it->second.screenX)) + ((mouseY - it->second.screenY) * (mouseY - it->second.screenY));
        if(square < minSquare) {
            minSquare = square;
            minRowID = it->first;
        }
    }

    return minRowID;
}

bool RadarWindow::lookup(int rowid, DataPoint &data) {
    std::map<int, DataPoint>::iterator it;
    it = pointCache.find(rowid);
    if(it != pointCache.end()) {
        data = it->second;
        return true;
    }
    return false;
}

int RadarWindow::handle(int event) {
    switch(event) {
        case FL_KEYDOWN:
        case FL_SHORTCUT: {
            int key = Fl::event_key();
            if(key == (FL_F + 1)) {
            }
            /*
            if(key == 'h') {
                ++curPoint;
                redraw();
                return 1;
            }
            else if(key == 'l') {
                --curPoint;
                redraw();
                return 1;
            }
            */
            if(key == FL_Down) {
                insideAngle += 5;
                completeRedraw = true;
                redraw();
                return 1;
            }
            else if(key == FL_Up) {
                insideAngle -= 5;
                completeRedraw = true;
                redraw();
                return 1;
            }
            if(key == 'w') {
                originY -= 5.0;
                completeRedraw = true;
                redraw();
                return 1;
            }
            else if(key == 's') {
                originY += 5.0;
                completeRedraw = true;
                redraw();
                return 1;
            }
            if(key == 'a') {
                pointCache.clear();
                if(scale < 1000.0) {
                    scale += 0.5;
                }
                completeRedraw = true;
                redraw();
                return 1;
            }
            else if(key == 'd') {
                pointCache.clear();
                if(scale > 0.0) {
                    scale -= 0.5;
                }
                completeRedraw = true;
                redraw();
                return 1;
            }
            return 1;
        }
        case FL_PUSH:
            if(Fl::event_button() == 1) {
                curPointRowID = rowidOfClosestPoint(Fl::event_x_root() - x(), Fl::event_y_root() - y());
                redraw();
            }
            return 1;
        default:
            return Fl_Window::handle(event);
    }
}

void RadarWindow::draw() {
    fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), FL_BLACK);
    fl_color(FL_GREEN);

    // height of line's second point
    // is (1/2) *  width * tan(outside angle converted to radians)
    // outside angle = (180.0 - inside angle) / 2
    float outsideAngle = (180.0 - insideAngle) / 2;
    outsideAngle *= (M_PI / 180.0); // convert to radians
    float yPos = w() * 0.5 * tan(outsideAngle);

    fl_line(0, originY, w(), originY);
    fl_line(originX, 0, originX, h());
    /*
    fl_line(originX, originY, 0, originY - yPos);
    fl_line(originX, originY, w(), originY - yPos);
    */

    char textBuffer[100];

    for(int i = 0; i < MAX_ARCS; i++) {
        fl_begin_line();
        fl_arc(originX, originY, (float)(i + 1) * radius, (180.0 / M_PI) * (0 + outsideAngle), (180.0 - ((180.0 / M_PI) * outsideAngle)));
        fl_end_line();
    }

    int lastValidIndex = -1;
    fl_color(FL_RED);

    int startPoint = 0;

    if(!rowData) {
        printf("no rowdata in radarwindow draw()\n");
        exit(0);
    }
    std::vector<Row>::iterator it;

    for(it = rowData->begin(); it != rowData->end(); it++) {
        if(strcmp(it->cols[1], RADAR_TYPE) != 0) {
            continue;
        }

        int screenX, screenY;
        computeCoords(it, screenX, screenY);
        fl_circle(screenX, screenY, 1);
    }
    completeRedraw = false;

    if(curPointRowID != -1) {
        // draw the laser sensor line
        fl_color(FL_YELLOW);
        fl_line(originX, originY, pointCache.find(curPointRowID)->second.screenX, pointCache.find(curPointRowID)->second.screenY);
        fl_color(FL_RED);
    }

    fl_color(FL_GREEN);

    fl_font(FL_HELVETICA_BOLD, 16);
    // draw the scale markers
    int textHeight = 0, textWidth = 0;
    for(int i = 0; i < MAX_ARCS; i++) {
        sprintf(textBuffer, "%.1f", (i+1) * scale);
        fl_measure(textBuffer, textWidth, textHeight, 0);
        fl_draw(textBuffer, originX - textWidth / 2.0, originY - ((float)(i+1) * radius) - 1.0);
    }

    /*
    if(data[curPointIndex].valid) {
        fl_measure(textBuffer, textWidth, textHeight, 0);
        sprintf(textBuffer, "%s %d%c, %fmm", "Current point:", curPointIndex, 0x00B0, data[curPointIndex].distance);
        fl_draw(textBuffer, 0, textHeight);
    }
    */

}
