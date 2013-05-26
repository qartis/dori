#include <FL/Fl.H>
#include <math.h>
#include <FL/Fl_Dial.H>
#include <FL/fl_draw.H>

#include "fl_gauge.h"

Fl_Gauge::Fl_Gauge(int x, int y, int w, int h, const char *l)
    : Fl_Dial(x,y,w,h,l) 
{
    selection_color(FL_RED);
    box(FL_FLAT_BOX);
    color(FL_BLACK);
}

void Fl_Gauge::draw(void) 
{
    int X = x();
    int Y = y();
    int W = w();
    int H = h();

    draw_box();

    X += Fl::box_dx(box());
    Y += Fl::box_dy(box());
    W -= Fl::box_dw(box());
    H -= Fl::box_dh(box());

    double frac = (value() - minimum()) / (maximum() - minimum());
    double angle = (angle2() - angle1()) * frac +  angle1();

    float cx = X + W/2.0f;
    float cy = Y + H/2.0f;

    float d = (float)((W < H)? W : H);

    double num_major_ticks = (maximum() - minimum()) / major_step_size;
    double num_minor_ticks = (maximum() - minimum()) / minor_step_size;

    double minor_angle = (angle2() - angle1()) / num_minor_ticks;
    double major_angle = (angle2() - angle1()) / num_major_ticks;
    int i;

/* minor ticks */
    fl_push_matrix();
    fl_color(FL_WHITE);
    fl_translate(cx, cy);
    fl_scale(d, d);
    fl_rotate(-angle1());
    for (i = 0; i <= num_minor_ticks; i++) {
        fl_begin_line();
        fl_vertex(0, 0.40);
        fl_vertex(0, 0.45);
        fl_end_line();
        fl_rotate(-minor_angle);
    }
    fl_pop_matrix();

/* major ticks */
    fl_push_matrix();
    fl_color(FL_WHITE);
    fl_line_style(FL_SOLID, 3);
    fl_translate(cx, cy);
    fl_scale(d, d);
    fl_rotate(-angle1());
    for (i = 0; i <= num_major_ticks; i++) {
        fl_begin_line();
        fl_vertex(0, 0.40);
        fl_vertex(0, 0.47);
        fl_end_line();
        fl_rotate(-major_angle);
    }
    fl_pop_matrix();

/* tick labels */
    char buf[128];
    fl_color(FL_WHITE);
    for (i = 0; i <= num_major_ticks; i++) {
        snprintf(buf, sizeof(buf), "%.0lf", minimum() + i * major_step_size);
        double angle = 360.0 + -angle1() - 90 - i * major_angle;
        double rad = angle * M_PI/180.0;
        int width = 0;
        int height = 0;
        fl_measure(buf, width, height);
        if (width < 1) {
            width = 9;
        }
        if (height < 1) {
            height = 16;
        }
        double radius = d / 3;
        double x = cx + radius * cos(rad);
        double y = cy - radius * sin(rad);
        if (rad < M_PI/2.0 || rad > 3 * M_PI/2.0) {
            x = cx + (radius - width/2.0) * cos(rad);
        }
        x -= width/2.0;
        fl_draw(buf, x, y + height/2.0);
    }

    fl_push_matrix();
    fl_translate(cx, cy);
    fl_scale(d, d);

    fl_color(FL_WHITE);

    fl_rotate(-angle);

/* arrow */
    fl_begin_polygon();
    fl_vertex(-0.02, 0.03);
    fl_vertex(-0.02, 0.30);
    fl_vertex( 0.00, 0.35);
    fl_vertex( 0.02, 0.30);
    fl_vertex( 0.02, 0.03);
    fl_end_polygon();

/* border */
    fl_begin_loop();
    fl_circle(0.0, 0.0, 0.48);
    fl_end_loop();

/* center spindle */
    fl_color(FL_GRAY);
    fl_line_style(FL_SOLID, 5);
    fl_begin_loop();
    fl_circle(0.0, 0.0, 0.04);
    fl_end_loop();

    fl_pop_matrix();

    draw_label();
}

int Fl_Gauge::handle(int e)
{
    switch (e) {
    case FL_PUSH:
    case FL_DRAG:
        return 1;
    default:
        return Fl_Dial::handle(e);
    }
}
