/* 
   Copyright 2006-2008 by the Yury Fedorchenko.
   
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with main.c; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
  
  Please report all bugs and problems to "yuryfdr@users.sourceforge.net".
   
*/
#include "GlArcballWindow.h"
#include <GL/gl.h>
#include <fltk/x.h>
#include <fltk/Cursor.h>
#include <fltk/xpmImage.h>

#include <stdio.h>

#include "../data/move.xpm"
#include "../data/rot.xpm"
#include "../data/rot_x.xpm"
#include "../data/rot_y.xpm"
#include "../data/rot_z.xpm"
#include "../data/zoom.xpm"
#include "../data/zoom_w.xpm"

namespace{
  fltk::Cursor *cur_move,*cur_rotate,*cur_rotate_x,*cur_rotate_y,*cur_rotate_z,
               *cur_zoom,*cur_zoom_w;
  bool cur_init=false;
};
void fltk::ext::GlArcballWindow::cursor(const Cursors3D cur){
  switch(cur){
    case CursorMove:
      fltk::Widget::cursor(cur_move);
      break;
    case CursorZoom:
      fltk::Widget::cursor(cur_zoom);
      break;
    case CursorZoomW:
      fltk::Widget::cursor(cur_zoom_w);
      break;
    case CursorRotateX:
      fltk::Widget::cursor(cur_rotate_x);
      break;
    case CursorRotateY:
      fltk::Widget::cursor(cur_rotate_y);
      break;
    case CursorRotateZ:
      fltk::Widget::cursor(cur_rotate_z);
      break;
    case CursorRotate:
      fltk::Widget::cursor(cur_rotate);
      break;
    case CursorDefault:
    default:
      fltk::Widget::cursor(fltk::CURSOR_ARROW);
  }
}
using namespace mvct;

void fltk::ext::GlArcballWindow::init()
{
  if(!fltk::xdisplay)fltk::open_display();
  if(!cur_init){
    xpmImage pmove(move_xpm);
    cur_move   = fltk::cursor(&pmove,15,15);
    xpmImage protate(rot_xpm);
    cur_rotate = fltk::cursor(&protate,15,15);
    xpmImage protatex(rot_x_xpm);
    cur_rotate = fltk::cursor(&protatex,15,15);
    xpmImage protatey(rot_y_xpm);
    cur_rotate = fltk::cursor(&protatey,15,15);
    xpmImage protatez(rot_z_xpm);
    cur_rotate = fltk::cursor(&protatez,15,15);
    xpmImage pzoom(zoom_xpm);
    cur_zoom_w = fltk::cursor(&pzoom,15,15);
    xpmImage pzoomw(zoom_w_xpm);
    cur_zoom_w = fltk::cursor(&pzoomw,15,15);
    cur_init=true;
  }
}


void fltk::ext::GlArcballWindow::draw_overlay()
{
  draw_zoom_frame();
}

int fltk::ext::GlArcballWindow::handle(int event)
{
  if (event == PUSH || event == RELEASE || event == DRAG) {
    if (event_button() == 1) {
      if (PUSH == event) {
	      cursor(CursorZoomW);
        b_zoom_frame = 1;
        mouse.x = d.x = event_x();
        mouse.y = d.y = event_y();
      }
      if (DRAG == event) {
        b_zoom_frame = 1;
        d.x = event_x();
        d.y = event_y();
      }
      if (RELEASE == event) {
	      cursor(CursorDefault);
        b_zoom_frame = 0;
        d.x = event_x();
        d.y = event_y();
        redraw();

        double sx =::fabs(d.x - mouse.x);
        double sy =::fabs(d.y - mouse.y);
        double zx;

        if (sx > 5 && sy > 5 && sx / sy < 13. && sy / sx < 13.) {
          if (sx / sy >= (double) w() / (double) h())
            zx = (double) w() / sx;
          else
            zx = (double) h() / sy;
          transform.translate(2. * Rmax * (w() - (mouse.x + d.x)) / 2. / (double) h(), -2. * Rmax * (h() - (mouse.y + d.y)) / 2. / (double) h(), 0.);
          transform.scale(zx, zx, zx);
        }
      }
      redraw_overlay();
      return 1;
    } else if (event_button() == 2) {
      if (event == PUSH || event == RELEASE || event == DRAG) {
        if (event_state(SHIFT)){
          handle_pan(mvct::XY((double) event_x() / w(), (double) event_y() / h()), PUSH == event, RELEASE == event);
        /*else if (( event_button() == 3 ) && ) handle_dolly(mvct::XY((double)event_x()/w(),(double)event_y()/h()),PUSH==event,RELEASE==event); */
        }else if (event_state(CTRL)){
          handle_zoom(mvct::XY((double) event_x() / w(), (double) event_y() / h()), PUSH == event, RELEASE == event);
        }else{
          handle_rotate(mvct::XY((double) event_x() / w(), (double) event_y() / h()), PUSH == event, RELEASE == event);
        }
        redraw();
        return 1;
      }
    }
  }
  if (MOUSEWHEEL == event) {
    if (event_dy() < 0)
      transform. /*post_ */ scale(XYZ(.9, .9, .9));
    else if (event_dy() > 0)
      transform. /*post_ */ scale(XYZ(1.1, 1.1, 1.1));
    redraw();
    return 1;
  }
  return GlWindow::handle(event);
}


void fltk::ext::GlArcballWindow::stext(int size,GLfloat x, GLfloat y, GLfloat z,const char *str) const
{
  glPushAttrib(GL_LIGHTING_BIT);
  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_LIGHTING);
  glRasterPos3f(x, y, z);
  fltk::glsetfont(fltk::HELVETICA, size);
  fltk::gldrawtext(str);
  glPopAttrib();
}
