#include <FL/Fl.H>
#include <math.h>
#include <FL/Fl_Dial.H>
#include <limits.h>
#include <FL/fl_draw.H>
#include <FL/Fl_Tooltip.H>
#include <FL/Fl_Menu_Window.H>

#include "sparkline.h"

class TipWin : public Fl_Menu_Window {
    char tip[40];
public:
    TipWin():Fl_Menu_Window(1,1) {      // will autosize
        strcpy(tip, "X.XX");
        set_override();
        end();
    }
    void draw() {
        draw_box(FL_BORDER_BOX, 0, 0, w(), h(), Fl_Color(175));
        fl_color(FL_BLACK);
        fl_font(labelfont(), labelsize());
        fl_draw(tip, 3, 3, w()-6, h()-6, Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_WRAP));
    }
    void value(float f) {
        sprintf(tip, "%.2f", f);
        // Recalc size of window
        fl_font(labelfont(), labelsize());
        int W = w(), H = h();
        fl_measure(tip, W, H, 0);
        W += 8;
        size(W, H);
        redraw();
    }
};

Fl_Sparkline::Fl_Sparkline(int x, int y, int w, int h, const char *l)
    : Fl_Widget(x,y,w,h,l) 
{
    selection_color(FL_RED);
    box(FL_FLAT_BOX);
    color(0xfffbcfff);
    padding = 2;
    Fl_Group *save = Fl_Group::current();
    tip = new TipWin();
    tip->hide();
    Fl_Group::current(save);
}

int map(int value, int in_min, int in_max, int out_min, int out_max)
{
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void Fl_Sparkline::drawPoint(int x)
{
    if (x >= width || x < 0) {
        return;
    }
    int index = num_values * x / width;
    int value = map(values[index], values[min_index], values[max_index], height, 0);
    fl_point(Fl_Sparkline::x() + padding + x, y() + value + padding);
}

void Fl_Sparkline::drawCursor(void)
{
    static int prev_x = -1;

    int x = Fl::event_x() - Fl_Widget::x();

    if (prev_x > -1) {
        fl_color(color());

        fl_line_style(FL_SOLID, 3);
        fl_line(Fl_Widget::x() + padding + prev_x, y(), Fl_Widget::x() + padding + prev_x, y() + h());

        fl_color(FL_BLACK);

        drawPoint(prev_x - 1);
        drawPoint(prev_x);
        drawPoint(prev_x + 1);

        drawPeaks();

        prev_x = -1;
    }


    if (x < padding || x >= padding + width) {
        return;
    }

    x -= padding;

    int index = num_values * x / width;
    index = snap(index);

    x = index * width / num_values;

    int value = map(values[index], values[min_index], values[max_index], height, 0);
        
    fl_color(FL_BLUE);
    fl_rectf(Fl_Widget::x() + padding + x - 1, y() + value + padding - 1, 3, 3);
    fl_color(FL_RED);
    fl_line_style(FL_SOLID, 1);
    fl_line(Fl_Widget::x() + padding + x, y() + padding, Fl_Widget::x() + padding + x, y() + h() - padding);

    prev_x = x;
}

int Fl_Sparkline::snap(int index)
{
    int i;

    int peak_dist = INT_MAX;
    int peak_index = -1;
    int trough_dist = INT_MAX;
    int trough_index = -1;

    for (i = 0; i < num_peaks; i++) {
        int dist = abs(index - peak_indices[i]);
        if (dist < peak_dist) {
            peak_dist = dist;
            peak_index = i;
        }
    }

    for (i = 0; i < num_troughs; i++) {
        int dist = abs(index - trough_indices[i]);
        if (dist < trough_dist) {
            trough_dist = dist;
            trough_index = i;
        }
    }

    if (trough_dist < peak_dist) {
        if (trough_dist < 50) {
            return trough_indices[trough_index];
        }
    } else {
        if (peak_dist < 50) {
            return peak_indices[peak_index];
        }
    }

    if (index < 50) {
        return 0;
    }

    if (index > num_values - 50) {
        return num_values - 1;
    }

    return index;
}

void Fl_Sparkline::draw(void) 
{
    int i;

    if (damage() == FL_DAMAGE_USER1) {
        int index = num_values * (Fl::event_x() - x() - padding) / width;
        index = snap(index);
        tip->position(Fl::event_x_root() + 10, Fl::event_y_root() + 10);
        tip->value(values[index]);
        tip->show();

        drawCursor();
        return;
    }

    width = w() - padding * 2;
    height = h() - padding * 2;


    draw_box();

    int index = num_values * (Fl::event_x() - padding) / width;
    index = snap(index);
    tip->position(Fl::event_x_root() + 10, Fl::event_y_root() + 10);
    tip->value(values[index]);
    tip->show();

    fl_color(FL_BLACK);
    for (i = 0; i < width; i++) {
        drawPoint(i);
    }

    drawPeaks();

    draw_label();

}

void Fl_Sparkline::drawPeaks(void)
{
    int i;
    int position;

    fl_color(FL_RED);
    for (i = 0; i < num_peaks; i++) {
        position = width * peak_indices[i] / num_values;
        fl_rectf(x() + padding + position, y() + padding, 3, 3);
    }

    for (i = 0; i < num_troughs; i++) {
        position = width * trough_indices[i] / num_values;
        fl_rectf(x() + padding + position, y() + padding + height - 3, 3, 3);
    }
}

void Fl_Sparkline::setValues(int *_values, int _num_values)
{
    int i;

    max_index = 0;
    min_index = 0;
    num_peaks = 0;
    num_troughs = 0;

    values = _values;
    num_values = _num_values;

    for (i = 0; i < num_values; i++) {
        if (values[i] < values[min_index]) {
            min_index = i;
        }
        if (values[i] > values[max_index]) {
            max_index = i;
        }
    }

    for (i = 0; i < num_values; i++) {
        if (values[i] == values[max_index]) {
            peak_indices[num_peaks++] = i;
        }
        if (values[i] == values[min_index]) {
            trough_indices[num_troughs++] = i;
        }
    }
}

int Fl_Sparkline::handle(int e)
{
    switch (e) {
    case FL_MOVE:
        damage(FL_DAMAGE_USER1);
        return 1;

    case FL_HIDE:
    case FL_LEAVE:
        tip->hide();
        return 1;

    case FL_ENTER:
    case FL_PUSH:
    case FL_DRAG:
        return 1;

    default:
        return Fl_Widget::handle(e);
    }
}
