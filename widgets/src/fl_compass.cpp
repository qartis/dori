#include <FL/Fl.H>
#include <FL/Fl_Dial.H>
#include <FL/fl_draw.H>

#include "fl_compass.h"

Fl_Compass::Fl_Compass(int x, int y, int w, int h, const char *l)
    : Fl_Dial(x,y,w,h,l)
{
  angles(0, 360);
  range(0.0, 360.0);
  selection_color(FL_RED);
}

void Fl_Compass::draw(void)
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

    fl_push_matrix();
    fl_translate(cx, cy);
    fl_scale(d, d);

    fl_color(active()?FL_BLACK:FL_INACTIVE_COLOR);

    fl_begin_loop();
    fl_circle(0.0, 0.0, 0.5);
    fl_end_loop();

    fl_begin_loop();
    fl_circle(0.0, 0.0, 0.45);
    fl_end_loop();

    fl_begin_line();
    fl_vertex(-0.5, 0.0);
    fl_vertex( 0.5, 0.0);
    fl_end_line();

    fl_begin_line();
    fl_vertex(0.0, -0.5);
    fl_vertex(0.0,  0.5);
    fl_end_line();

    fl_begin_line();
    fl_vertex(-0.15,-0.15);
    fl_vertex( 0.15, 0.15);
    fl_end_line();

    fl_begin_line();
    fl_vertex(-0.15, 0.15);
    fl_vertex( 0.15,-0.15);
    fl_end_line();

    fl_rotate(-angle);

    fl_begin_polygon();
    fl_vertex( 0.00,-0.43);
    fl_vertex(-0.04,-0.30);
    fl_vertex(-0.08, 0.00);
    fl_vertex(-0.04, 0.30);
    fl_vertex( 0.04, 0.30);
    fl_vertex( 0.08, 0.00);
    fl_vertex( 0.04,-0.30);
    fl_end_polygon();

    fl_color(active()?selection_color():fl_inactive(selection_color()));

    fl_begin_polygon();
    fl_vertex(-0.04, 0.30);
    fl_vertex( 0.00, 0.43);
    fl_vertex( 0.04, 0.30);
    fl_end_polygon();

    fl_pop_matrix();

    draw_label();
}

int Fl_Compass::handle(int e)
{
    switch (e) {
    case FL_PUSH:
    case FL_DRAG:
        return 1;
    default:
        return Fl_Dial::handle(e);
    }
}
