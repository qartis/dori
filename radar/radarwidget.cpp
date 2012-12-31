#include "radarwidget.h"
#include <FL/fl_draw.H>
#include <math.h>

#define MAX_ARCS 12

#define RADAR_TYPE "laser"

static void resetCallback(void *widget) {
    if(widget == NULL) {
        perror("widget = NULL in radar reset callback");
        return;
    }

    RadarWidget *radar = (RadarWidget *)widget;

    for(int i = 0; i < MAX_ANGLES; i++)
    {
        radar->data[i].valid = false;
        radar->data[i].changed = false;
        radar->data[i].screenX = 0.0;
        radar->data[i].screenY = 0.0;
    }

    radar->redraw();
}
static void dataCallback(void *data, void *widget) {
    // data should be a string something like "type|value1|value2"
    // for the radar this will look like "laser|45|12.31"

    if(widget == NULL) {
        perror("widget = NULL in radar data callback");
        return;
    }

    if(data == NULL) {
        perror("data = NULL in radar data callback");
        return;
    }

    RadarWidget *radar = (RadarWidget *)widget;

    char* buffer = (char*)data;
    char type[32];
    float angle = 0;
    float distance = 0;
    int time = 0;

    sscanf(buffer, "%[^,], %f, %f, %d", type, &angle, &distance, &time);

    if(strcmp(type, RADAR_TYPE) != 0) {
        printf("type: %s\n", type);
        return;
    }

    radar->data[(int)angle].valid = true;
    radar->data[(int)angle].changed = true;
    radar->data[(int)angle].distance = distance;

    radar->redraw();
}

RadarWidget::RadarWidget(int x, int y, int w, int h, const char *label) : Fl_Widget(x, y, w, h, label), tableViewWindow(w, h), tableView(0, 0, w, h)  {
    insideAngle = 90.0;
    originX = w / 2.0;
    originY = h;// / 2.0;
    completeRedraw = false;
    scale = 2.0;
    curPointIndex = 0;

    for(int i = 0; i < MAX_ANGLES; i++)
    {
        data[i].valid = false;
        data[i].changed = false;
        data[i].screenX = 0.0;
        data[i].screenY = 0.0;
    }

    tableViewWindow.position(x+w, y);
    tableView.parentWidget = this;
    tableView.widgetDataCallback = dataCallback;
    tableView.widgetResetCallback = resetCallback;
}

int RadarWidget::handle(int event) {
    switch(event) {
        case FL_KEYDOWN:
        case FL_SHORTCUT: {
            int key = Fl::event_key();
            if(key == (FL_F + 1)) {
                if(!tableViewWindow.shown()) {
                    tableViewWindow.show();
                }
                else {
                    tableViewWindow.hide();
                }
                return 1;
            }
            if(key == 'h') {
                int startPoint = curPointIndex;
                do {
                    curPointIndex++;

                    if(curPointIndex > MAX_ANGLES) {
                        curPointIndex = 0;
                    }
                } while(!data[curPointIndex].valid && curPointIndex != startPoint);
                redraw();
                return 1;
            }
            else if(key == 'l') {
                int startPoint = curPointIndex;
                do {
                    curPointIndex--;

                    if(curPointIndex < 0) {
                        curPointIndex = MAX_ANGLES;
                    }
                } while(!data[curPointIndex].valid && curPointIndex != startPoint);
                redraw();
                return 1;
            }
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
                if(scale < 1000.0) {
                    scale += 0.5;
                }
                completeRedraw = true;
                redraw();
                return 1;
            }
            else if(key == 'd') {
                if(scale > 0.0) {
                    scale -= 0.5;
                }
                completeRedraw = true;
                redraw();
                return 1;
            }
            return 1;
        }
        default:
            return Fl_Widget::handle(event);
    }
}

void RadarWidget::draw() {
    fl_draw_box(FL_FLAT_BOX, 0, 0, w(), h(), FL_BLACK);
    fl_color(FL_GREEN);

    // height of line's second point
    // is (1/2) *  width * tan(outside angle converted to radians)
    // outside angle = (180.0 - inside angle) / 2
    float outsideAngle = (180.0 - insideAngle) / 2;
    outsideAngle *= (M_PI / 180.0); // convert to radians
    float yPos = w() * 0.5 * tan(outsideAngle);

    float radius = w() / 16.0;
    float ratio = radius / scale;

    fl_line(originX, originY, 0, originY - yPos);
    fl_line(originX, originY, w(), originY - yPos);
    
    char textBuffer[100];

    for(int i = 0; i < MAX_ARCS; i++) {
        fl_begin_line();
        fl_arc(originX, originY, (float)(i + 1) * radius, (180.0 / M_PI) * (0 + outsideAngle), (180.0 - ((180.0 / M_PI) * outsideAngle)));
        fl_end_line();
    }

    int lastValidIndex = -1;
    fl_color(FL_RED);

    int startPoint = 0;

    for(int i = startPoint; i < MAX_ANGLES; i++) {
        if(data[i].valid) {
            // y = b sin alpha
            // x = b cos alpha
            // alpha = angle of range finder
            // b = distance

            // only recalculate position if we need to
            if(data[i].changed || completeRedraw) {
                if(data[i].changed) { // draw the sensor line to the most recently added data point
                    curPointIndex = i;
                }

                data[i].changed = false;
                data[i].screenX = originX + (ratio * (data[i].distance * (cos((float)i * (M_PI / 180.0)))));
                data[i].screenY = originY - (ratio * (data[i].distance * (sin((float)i * (M_PI / 180.0)))));

            }

            fl_circle(data[i].screenX, data[i].screenY, 1);

            if(lastValidIndex >= 0) {
                fl_line(data[i].screenX, data[i].screenY, data[lastValidIndex].screenX, data[lastValidIndex].screenY);
            }

            lastValidIndex = i;
        }
    }

    completeRedraw = false;

    if(data[curPointIndex].valid) {
        // draw the laser sensor line
        fl_color(FL_YELLOW);
        fl_line(originX, originY, data[curPointIndex].screenX, data[curPointIndex].screenY);
        fl_color(FL_RED);
    }

    fl_color(FL_GREEN);

    // draw the scale markers
    int textHeight = 0, textWidth = 0;
    for(int i = 0; i < MAX_ARCS; i++) {
        sprintf(textBuffer, "%.1f", (i+1) * scale);
        fl_measure(textBuffer, textWidth, textHeight, 0);
        fl_draw(textBuffer, originX - textWidth / 2.0, originY - ((float)(i+1) * radius) - 1.0);
    }

    if(data[curPointIndex].valid) {
        fl_measure(textBuffer, textWidth, textHeight, 0);
        sprintf(textBuffer, "%s %d%c, %fmm", "Current point:", curPointIndex, 0x00B0, data[curPointIndex].distance);
        fl_draw(textBuffer, 0, textHeight);
    }

}
