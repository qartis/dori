#include "radarwidget.h"
#include <FL/fl_draw.H>
#include <math.h>

RadarWidget::RadarWidget(int x, int y, int w, int h, const char *label)
    : Fl_Widget(x, y, w, h, label) {
        insideAngle = 30.0;
        for(int i = 0; i < 180; i++)
        {
            data[i] = 0.0;
        }
    }

int RadarWidget::handle(int event) {
    switch(event) {
        case FL_KEYDOWN: {
            int key = Fl::event_key();
            if(key == FL_Up) {
                insideAngle += 5;
                redraw();
            }
            else if(key == FL_Down) {
                insideAngle -= 5;
                redraw();
            }
        }
        break;
    }
}

void RadarWidget::insertDataPoint(int index, float distance) {
    printf("setting %d to %f\n", index, distance);
    data[index] = distance;
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
    fl_line(w()/2.0, h(), 0, h()-yPos);
    fl_line(w()/2.0, h(), w(), h()-yPos);

    float radius = w() / 8.0;
    float scale = 10.0; // 10mm per arc
    float ratio = radius / scale;

    for(int i = 0; i < 8; i++) {
        fl_begin_line();
        fl_arc(w()/2.0, h(), (float)i * radius, (180.0 / M_PI) * (0 + outsideAngle), (180.0 - ((180.0 / M_PI) * outsideAngle)));
        fl_end_line();
    }

    for(int i = 0; i < 180; i++) {
        if(data[i] > 0.0)
        {
            // y = b sin alpha
            // x = b cos alpha
            // alpha = angle of range finder
            // b = distance
            float pointX = (w() / 2.0) + (data[i] * (cos((float)i * (M_PI / 180.0))));
            float pointY = h() - (ratio * (data[i] * (sin((float)i * (M_PI / 180.0)))));
            printf("(%f, %f)\n", pointX, pointY);
            fl_circle(pointX, pointY, 5);
        }
    }
}

void RadarWidget::draw() {
    drawBase();
}

