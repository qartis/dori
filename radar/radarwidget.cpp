#include "radarwidget.h"
#include <FL/fl_draw.H>
#include <math.h>

#define MAX_ANGLES 360

RadarWidget::RadarWidget(int x, int y, int w, int h, const char *label) : Fl_Widget(x, y, w, h, label) {
    insideAngle = 90.0;
    originX = w / 2.0;
    originY = h / 2.0;
    completeRedraw = false;

    for(int i = 0; i < MAX_ANGLES; i++)
    {
        data[i].valid = false;
        data[i].changed = false;
        data[i].screenX = 0.0;
        data[i].screenY = 0.0;
    }

    fl_font(FL_TIMES, 12);
}

int RadarWidget::handle(int event) {
    switch(event) {
        case FL_KEYDOWN: {
            int key = Fl::event_key();
            if(key == FL_Up) {
                insideAngle += 5;
                completeRedraw = true;
                redraw();
            }
            else if(key == FL_Down) {
                insideAngle -= 5;
                completeRedraw = true;
                redraw();
            }
            if(key == 'w') {
                originY -= 5.0;
                completeRedraw = true;
                redraw();
            }
            else if(key == 's') {
                originY += 5.0;
                completeRedraw = true;
                redraw();
            }

        }
        break;
    }
}

void RadarWidget::insertDataPoint(int index, float distance) {
    data[index].valid = true;
    data[index].changed = true;
    data[index].distance = distance;
    redraw();
}

void RadarWidget::drawBase() {
    fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), FL_BLACK);
    fl_color(FL_GREEN);

    // height of line's second point
    // is (1/2) *  width * tan(outside angle converted to radians)
    // outside angle = (180.0 - inside angle) / 2
    float outsideAngle = (180.0 - insideAngle) / 2;
    outsideAngle *= (M_PI / 180.0); // convert to radians
    float yPos = w() * 0.5 * tan(outsideAngle);

    float radius = w() / 16.0;
    float scale = 10.0; // 10mm per arc
    float ratio = radius / scale;

    fl_line(originX, originY, 0, originY - yPos);
    fl_line(originX, originY, w(), originY - yPos);

    char scaleBuffer[10];

    for(int i = 0; i < 8; i++) {
        fl_begin_line();
        fl_arc(originX, originY, (float)(i + 1) * radius, (180.0 / M_PI) * (0 + outsideAngle), (180.0 - ((180.0 / M_PI) * outsideAngle)));
        fl_end_line();

    }

    int lastValidIndex = -1;

    for(int i = 0; i < MAX_ANGLES; i++) {
        if(data[i].valid)
        {
            fl_color(FL_RED);
            // y = b sin alpha
            // x = b cos alpha
            // alpha = angle of range finder
            // b = distance
            if(data[i].changed || completeRedraw) {
                data[i].changed = false;
                data[i].screenX = originX + (ratio * (data[i].distance * (cos((float)i * (M_PI / 180.0)))));
                data[i].screenY = originY - (ratio * (data[i].distance * (sin((float)i * (M_PI / 180.0)))));
                fl_circle(data[i].screenX, data[i].screenY, 1);
            }

            if(lastValidIndex >= 0)
            {
                fl_line(data[i].screenX, data[i].screenY, data[lastValidIndex].screenX, data[lastValidIndex].screenY);
            }

            lastValidIndex = i;
        }
    }

    completeRedraw = false;

    fl_color(FL_GREEN);

    // draw the scale markers last
    for(int i = 0; i < 8; i++) {
        sprintf(scaleBuffer, "%.1f", (i+1) * scale);
        int textHeight = 0, textWidth = 0;
        fl_measure(scaleBuffer, textHeight, textWidth, 0);
        fl_draw(scaleBuffer, originX - textWidth, originY - ((float)(i+1) * radius) - 1.0);

    }
}

void RadarWidget::draw() {
    drawBase();
}

