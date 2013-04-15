#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Table_Row.H>
#include <FL/Fl_Input.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>
#include <math.h>
#include <map>
#include <vector>
#include <limits.h>
#include "ctype.h"
#include "../mainwindow/queryinput.h"
#include "../row.h"
#include "../mainwindow/table.h"
#include "radarwindow.h"

#define MAX_ARCS 8
#define CUSTOM_FONT 69

#define RADAR_TYPE "laser"

RadarWindow::RadarWindow(int x, int y, int w, int h, const char *label) : Fl_Double_Window(x, y, w, h, label) {
    Fl::set_font(CUSTOM_FONT, "OCRB");
    insideAngle = 360.0;
    scale = 2.0;
    curPointRowID = -1;
    table = NULL;
}

void RadarWindow::computeCoords(std::vector<Row>::iterator it, int &screenX, int &screenY) {
    if(!table || rowidCol < 0 || typeCol < 0 || angleCol < 0 || distanceCol < 0) {
        return;
    }
    int rowid = atoi(it->cols[rowidCol]);

    DataPoint data;

    if(lookup(rowid, data)) {
        screenX = data.screenX;
        screenY = data.screenY;
    }
    else {
        float angle, distance, ratio;

        ratio = radius / scale;
        angle = atof(it->cols[angleCol]);
        distance = atof(it->cols[distanceCol]);

        screenX = originX + (ratio * (distance * (cos((float)angle * (M_PI / 180.0)))));
        screenY = originY - (ratio * (distance * (sin((float)angle * (M_PI / 180.0)))));

        DataPoint newPoint;
        newPoint.screenX = screenX;
        newPoint.screenY = screenY;
        newPoint.distance = distance;

        pointCache[rowid] = newPoint;
    }
}

int RadarWindow::rowidOfClosestPoint(int mouseX, int mouseY) {
    std::map<int, DataPoint>::iterator it;
    int minRowID, minSquare;
    int square;

    minSquare = INT_MAX;
    minRowID = 0;

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
    case FL_PUSH:
        if(Fl::event_button() == 1) {
            curPointRowID = rowidOfClosestPoint(Fl::event_x_root() - x(), Fl::event_y_root() - y());
            redraw();
        }
        return 1;
    case FL_MOUSEWHEEL:
        pointCache.clear();
        scale += Fl::event_dy();
        redraw();
        return 1;
        break;
    default:
        return Fl_Double_Window::handle(event);
    }
}

void RadarWindow::resize(int x, int y, int w, int h) {
    pointCache.clear();
    return Fl_Double_Window::resize(x, y, w, h);
}

void RadarWindow::draw() {
    originX = w() / 2.0;
    originY = h() / 2.0;

    radius = w() < h() ? w() : h();
    radius /= 15.0;

    // height of line's second point
    // is (1/2) *  width * tan(outside angle converted to radians)
    // outside angle = (180.0 - inside angle) / 2
    float outsideAngle = (180.0 - insideAngle) / 2;
    outsideAngle *= (M_PI / 180.0); // convert to radians

    fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), FL_BLACK);
    fl_color(FL_GREEN);

    fl_line(0, originY, w(), originY);
    fl_line(originX, 0, originX, h());

    char textBuffer[100];

    for(int i = 0; i < MAX_ARCS; i++) {
        fl_begin_line();
        fl_arc(originX, originY, (float)(i + 1) * radius, (180.0 / M_PI) * (0 + outsideAngle), (180.0 - ((180.0 / M_PI) * outsideAngle)));
        fl_end_line();
    }

    fl_color(FL_RED);

    if(!table) {
        printf("no table\n");
        return;
    }

    std::vector<Row>::iterator it;
    int lastPointRowID = curPointRowID;
    curPointRowID = -1;

    rowidCol = typeCol = angleCol = distanceCol = -1;

    for(it = table->_rowdata.begin(); it != table->_rowdata.end(); it++) {
        for(unsigned i = 0; i < table->headers->size(); i++) {
            if(strncmp((*(table->headers))[i], "row", strlen("row")) == 0) {
                rowidCol = i;
            }
            else if(strcmp((*(table->headers))[i], "type") == 0) {
                typeCol = i;
            }
            else if(strcmp((*(table->headers))[i], "a") == 0 ||
                    strcmp((*(table->headers))[i], "angle") == 0) {
                angleCol = i;
            }
            else if(strcmp((*(table->headers))[i], "b") == 0 ||
                    strncmp((*(table->headers))[i], "dist", strlen("dist")) == 0) {
                distanceCol = i;
            }
        }

        if(typeCol < 0 || angleCol < 0 || distanceCol < 0) {
            break;
        }

        if(strcmp(it->cols[typeCol], RADAR_TYPE) != 0) {
            continue;
        }

        curPointRowID = lastPointRowID;

        int screenX, screenY;
        computeCoords(it, screenX, screenY);

        if(it->is_live_data) {
            fl_color(FL_YELLOW);
            fl_circle(screenX, screenY, 7);
        }

        fl_color(FL_RED);
        fl_circle(screenX, screenY, 5);

    }

    int textHeight = 0, textWidth = 0;

    if(curPointRowID != -1) {
        // draw the laser sensor line
        fl_color(FL_YELLOW);
        fl_line(originX, originY, pointCache.find(curPointRowID)->second.screenX, pointCache.find(curPointRowID)->second.screenY);
        fl_color(FL_RED);

        fl_font(CUSTOM_FONT, 12);
        fl_color(FL_GREEN);
        sprintf(textBuffer, "CUR PT DIST: %d", pointCache[curPointRowID].distance);
        fl_measure(textBuffer, textWidth, textHeight, 0);
        fl_draw(textBuffer, 5.0, 13.0);
    }

    fl_color(FL_GREEN);

    fl_font(FL_HELVETICA_BOLD, 16);
    // draw the scale markers
    for(int i = 0; i < MAX_ARCS; i++) {
        sprintf(textBuffer, "%.1f", (i+1) * scale);
        fl_measure(textBuffer, textWidth, textHeight, 0);
        fl_draw(textBuffer, originX - textWidth / 2.0, originY - ((float)(i+1) * radius) - 1.0);
    }

}
