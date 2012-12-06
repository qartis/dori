/*
   Copyright 2004-2009 by the Yury Fedorchenko.
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.
  
   Please report all bugs and problems to "yuryfdr@users.sourceforge.net".
   
*/

#include "FlGlArcballWindow.h"
#include <GL/gl.h>
#include <FL/x.H>
#include <FL/fl_draw.H>

#include "../flcursors/fltk1/Fl_Custom_Cursor.H"

#include "../data/move.xpm"
#include "../data/rot.xpm"
#include "../data/rot_x.xpm"
#include "../data/rot_y.xpm"
#include "../data/rot_z.xpm"
#include "../data/zoom.xpm"
#include "../data/zoom_w.xpm"

namespace{
  Fl_Custom_Cursor cur_move(move_xpm,15,15),cur_rotate(rot_xpm,15,15),cur_rotate_x(rot_x_xpm,15,15),cur_rotate_y(rot_y_xpm,15,15),
  cur_rotate_z(rot_z_xpm,15,15),
  cur_zoom(zoom_xpm,0,0),cur_zoom_w(zoom_w_xpm,0,0);
};

void FlGlArcballWindow::cursor(const Cursors3D cur){
  switch(cur){
    case CursorMove:
      cur_move.cursor(this);
      break;
    case CursorZoom:
      cur_zoom.cursor(this);
      break;
    case CursorZoomW:
      cur_zoom_w.cursor(this);
      break;
    case CursorRotateX:
      cur_rotate_x.cursor(this);
      break;
    case CursorRotateY:
      cur_rotate_y.cursor(this);
      break;
    case CursorRotateZ:
      cur_rotate_z.cursor(this);
      break;
    case CursorRotate:
      cur_rotate.cursor(this);
      break;
    case CursorDefault:
    default:
      Fl_Gl_Window::cursor(FL_CURSOR_ARROW);
  }
};

using namespace mvct;
void FlGlArcballWindow::draw_overlay ()
{
  draw_zoom_frame();
}

void FlGlArcballWindow::init(){
//  if(!fl_display)fl_open_display();
}

int FlGlArcballWindow::handle(int event) {
  switch (event) {
  case FL_FOCUS: return 1;
  case FL_KEYDOWN:
  case FL_SHORTCUT:
    switch (Fl::event_key()) {
    case FL_Home:
    case FL_KP + '7':
    case '*':
      reset(Fl::event_state(FL_SHIFT));
      redraw();
      return 1;
    case FL_KP + '+':
    case '+':
      transform.scale(XYZ(1.1,1.1,1.1));
      redraw();
      return 1;
    case FL_KP + '-':
    case '-':
      transform.scale(XYZ(.9,.9,.9));
      redraw();
      return 1;
    case FL_KP + '8':
      transform.translate(0, -Rmax * 0.1, 0.);
      redraw();
      return 1;
    case FL_KP + '2':
      transform.translate(0, Rmax * 0.1, 0.);
      redraw();
      return 1;
    case FL_KP + '4':
      transform.translate(Rmax * 0.1, 0, 0.);
      redraw();
      return 1;
    case FL_KP + '6':
      transform.translate(-Rmax * 0.1, 0, 0.);
      redraw();
      return 1;
    }  
  }
  {
    if ( Fl::event_button() == FL_LEFT_MOUSE || Fl::event_button1()){
      if(FL_PUSH==event){
	      cursor(CursorZoomW);
        b_zoom_frame=true;
        mouse.x = d.x = Fl::event_x();
        mouse.y = d.y = Fl::event_y();
      }else if(FL_DRAG==event){
        d.x = Fl::event_x();
        d.y = Fl::event_y();
      }else  if(FL_RELEASE==event && b_zoom_frame){
	      cursor(CursorDefault);
        b_zoom_frame=false;
        d.x = Fl::event_x();
        d.y = Fl::event_y();
        redraw();

        double sx =::fabs(d.x-mouse.x);
        double sy =::fabs(d.y-mouse.y);
        double zx;

        if(sx > 5 && sy > 5 && sx / sy < 13. && sy / sx < 13.){
          if (sx / sy >= (double) w () / (double) h ()) zx = (double) w () / sx;
          else zx = (double) h () / sy;
          transform.translate(2.*Rmax*(w()-(mouse.x+d.x))/2./(double)h(),
                              -2.*Rmax*(h()-(mouse.y + d.y))/2./(double)h(),0.);
          transform.scale (zx, zx, zx);
        }
      }
      redraw_overlay();
	    return 1;
    }else if ( Fl::event_button() == FL_MIDDLE_MOUSE || Fl::event_button2()){
      if ( event == FL_PUSH || event == FL_RELEASE || event == FL_DRAG ){
        if (Fl::event_state(FL_SHIFT)) handle_pan(mvct::XY((double)Fl::event_x()/w(),(double)Fl::event_y()/h()),FL_PUSH==event,FL_RELEASE==event);
        /*else if (( Fl::event_button() == FL_RIGHT_MOUSE ) && ) handle_dolly(mvct::XY((double)Fl::event_x()/w(),(double)Fl::event_y()/h()),FL_PUSH==event,FL_RELEASE==event);*/
        else if (Fl::event_state(FL_CTRL)) handle_zoom(mvct::XY((double)Fl::event_x()/w(),(double)Fl::event_y()/h()),FL_PUSH==event,FL_RELEASE==event);
        else handle_rotate(mvct::XY((double)Fl::event_x()/w(),(double)Fl::event_y()/h()),FL_PUSH==event,FL_RELEASE==event);
        redraw();
        return 1;
      }
    }
  }
  if(FL_MOUSEWHEEL==event){
    if(Fl::event_dy()<0)transform./*post_*/scale(XYZ(.9,.9,.9));
    else if(Fl::event_dy()>0)transform./*post_*/scale(XYZ(1.1,1.1,1.1));
    redraw();
    return 1;
  }
  return Fl_Gl_Window::handle(event);
}

void FlGlArcballWindow::stext(int size,GLfloat x, GLfloat y, GLfloat z,const char *str)const{
  glRasterPos3f (x, y, z);
  gl_font(FL_HELVETICA,size);
  gl_draw(str);
}

