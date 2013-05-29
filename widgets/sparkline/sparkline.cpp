#include <FL/Fl.H>
#include <math.h>
#include <FL/Fl_Dial.H>
#include <FL/fl_draw.H>

#include "sparkline.h"

Fl_Sparkline::Fl_Sparkline(int x, int y, int w, int h, const char *l)
    : Fl_Widget(x,y,w,h,l) 
{
    selection_color(FL_RED);
    box(FL_FLAT_BOX);
    color(FL_WHITE);
    padding = 10;
}

int map(int value, int in_min, int in_max, int out_min, int out_max)
{
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void Fl_Sparkline::drawPoint(int x)
{
        int index = num_values * x / width;
        int value = map(values[index], values[min_index], values[max_index], height, 0);
        fl_point(padding + x, value + padding);
}

void Fl_Sparkline::drawCursor(void)
{
    static int prev_x = -1;

    int x = Fl::event_x();

    if (prev_x > -1) {
        fl_color(FL_WHITE);

        fl_line_style(FL_SOLID, 3);
        fl_line(padding + prev_x, 0, padding + prev_x, h());

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

    fl_color(FL_BLUE);
    int index = num_values * x / width;

    int value = map(values[index], values[min_index], values[max_index], height, 0);
        
    fl_rectf(padding + x - 1, value + padding - 1, 3, 3);

    prev_x = x;
}

void Fl_Sparkline::draw(void) 
{
    int i;

    if (damage() == FL_DAMAGE_USER1) {
        drawCursor();
        return;
    }

    width = w() - padding * 2;
    height = h() - padding * 2;

    draw_box();

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
        fl_rectf(padding + position, padding, 3, 3);
    }

    for (i = 0; i < num_troughs; i++) {
        position = width * trough_indices[i] / num_values;
        fl_rectf(padding + position, padding + height - 3, 3, 3);
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

    case FL_ENTER:
    case FL_PUSH:
    case FL_DRAG:
        return 1;

    default:
        return Fl_Widget::handle(e);
    }
}
