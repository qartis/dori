/* 
   Copyright 2007-2009 by the Yury Fedorchenko.
   
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

#include "gtkmm_arcball.h"
#include <iostream>
#include <GL/gl.h>

#include "../data/move.xpm"
#include "../data/rot.xpm"
#include "../data/rot_x.xpm"
#include "../data/rot_y.xpm"
#include "../data/rot_z.xpm"
#include "../data/zoom.xpm"
#include "../data/zoom_w.xpm"

#if 0
#include "../Cobra/cobra2nls.h"
#else
#define _(String) (String)
#endif

using std::cerr;
using std::endl;
using namespace mvct;

namespace{
  bool cur_init=false;
  Gdk::Cursor cur_move;
  Gdk::Cursor cur_rotate;
  Gdk::Cursor cur_zoom;
  Gdk::Cursor cur_rotate_x;
  Gdk::Cursor cur_rotate_y;
  Gdk::Cursor cur_rotate_z;
  Gdk::Cursor cur_zoom_w;
};

Gtk::ArcballWidget::ArcballWidget(const Glib::RefPtr<const Gdk::GL::Config>& config)
    : Gtk::GL::DrawingArea(config)
  {
    set_flags(get_flags()|Gtk::CAN_FOCUS); 
    add_events(Gdk::POINTER_MOTION_MASK|
    //Gdk::BUTTON_MOTION_MASK 	|
    Gdk::BUTTON1_MOTION_MASK 	|
    Gdk::BUTTON2_MOTION_MASK 	|
    Gdk::BUTTON3_MOTION_MASK 	|
    Gdk::BUTTON_PRESS_MASK 	|
    Gdk::BUTTON_RELEASE_MASK | Gdk::SCROLL_MASK	|
		Gdk::KEY_PRESS_MASK|Gdk::ENTER_NOTIFY_MASK|
		Gdk::LEAVE_NOTIFY_MASK|Gdk::FOCUS_CHANGE_MASK);
		
    if(!cur_init){
      cur_move  =Gdk::Cursor(Gdk::Display::get_default(),Gdk::Pixbuf::create_from_xpm_data(move_xpm),15,15);
      cur_rotate=Gdk::Cursor(Gdk::Display::get_default(),Gdk::Pixbuf::create_from_xpm_data(rot_xpm),15,15);
      cur_rotate_x=Gdk::Cursor(Gdk::Display::get_default(),Gdk::Pixbuf::create_from_xpm_data(rot_x_xpm),15,15);
      cur_rotate_y=Gdk::Cursor(Gdk::Display::get_default(),Gdk::Pixbuf::create_from_xpm_data(rot_y_xpm),15,15);
      cur_rotate_z=Gdk::Cursor(Gdk::Display::get_default(),Gdk::Pixbuf::create_from_xpm_data(rot_z_xpm),15,15);
      cur_zoom  =Gdk::Cursor(Gdk::Display::get_default(),Gdk::Pixbuf::create_from_xpm_data(zoom_xpm),0,0);
      cur_zoom_w=Gdk::Cursor(Gdk::Display::get_default(),Gdk::Pixbuf::create_from_xpm_data(zoom_w_xpm),0,0);
    }
    cur_init=true;
}

void Gtk::ArcballWidget::cursor(const Cursors3D cur){
  switch(cur){
    case CursorMove:
      get_window()->set_cursor(cur_move);
      break;
    case CursorZoom:
      get_window()->set_cursor(cur_zoom);
      break;
    case CursorZoomW:
      get_window()->set_cursor(cur_zoom_w);
      break;
    case CursorRotateX:
      get_window()->set_cursor(cur_rotate_x);
      break;
    case CursorRotateY:
      get_window()->set_cursor(cur_rotate_y);
      break;
    case CursorRotateZ:
      get_window()->set_cursor(cur_rotate_z);
      break;
    case CursorRotate:
      get_window()->set_cursor(cur_rotate);
      break;
    default:
      get_window()->set_cursor();
  }
};

bool Gtk::ArcballWidget::on_configure_event(GdkEventConfigure* event)
{
  if(!is_realized())return false;
  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();
  if(!glwindow)return false;
  // *** OpenGL BEGIN ***
  if (!glwindow->gl_begin(get_gl_context())) return false;
  reshape();
  // *** OpenGL END ***
  return true;
}

bool Gtk::ArcballWidget::on_button_press_event(GdkEventButton* event){
  if(event->button==2){
    if(event->state & GDK_SHIFT_MASK){
	    handle_pan(XY(event->x/get_width(),event->y/get_height()),true,false);
    }else if(event->state & GDK_CONTROL_MASK){
      handle_zoom(XY(event->x/get_width(),event->y/get_height()),true,false);
    }else if(event->state & GDK_SUPER_MASK){
      handle_dolly(XY(event->x/get_width(),event->y/get_height()),false,false);
    }else{
      handle_rotate(XY(event->x/get_width(),event->y/get_height()),true,false);
    }
  }else if(event->button==1){
    b_zoom_frame=true;
    mouse.x=d.x=event->x;
    mouse.y=d.y=event->y;
  }
  queue_draw();
  return true;
}
bool Gtk::ArcballWidget::on_button_release_event (GdkEventButton* event){
  if(event->button==2){
    if(event->state & GDK_SHIFT_MASK)handle_pan(XY(event->x/get_width(),event->y/get_height()),false,true);
    else if(event->state & GDK_CONTROL_MASK)handle_zoom(XY(event->x/get_width(),event->y/get_height()),false,true);
    else if(event->state & GDK_SUPER_MASK)handle_dolly(XY(event->x/get_width(),event->y/get_height()),false,false);
    else handle_rotate(XY(event->x/get_width(),event->y/get_height()),false,true);
  }else if(event->button==1 && b_zoom_frame){
    b_zoom_frame=false;
    double sx =::fabs(d.x-mouse.x);
    double sy =::fabs(d.y-mouse.y);
    double zx;
      if(sx > 5 && sy > 5 && sx / sy < 13. && sy / sx < 13.){
         if (sx / sy >= get_width() / get_height()) zx = get_width() / sx;
         else zx = get_height() / sy;
         transform.translate(2.*Rmax*(get_width()-(mouse.x+d.x))/2./get_height(),
                            -2.*Rmax*(get_height()-(mouse.y + d.y))/2./get_height(),0.);
         transform.scale (zx, zx, zx);
      }
  }
  get_window()->set_cursor();
  queue_draw();
  return true;
}
bool Gtk::ArcballWidget::on_motion_notify_event (GdkEventMotion* event){
  if(event->state & GDK_BUTTON2_MASK){
    if(event->state & GDK_SHIFT_MASK)handle_pan(XY(event->x/get_width(),event->y/get_height()),false,false);
    else if(event->state & GDK_CONTROL_MASK)handle_zoom(XY(event->x/get_width(),event->y/get_height()),false,false);
    else if(event->state & GDK_SUPER_MASK)handle_dolly(XY(event->x/get_width(),event->y/get_height()),false,false);
    else handle_rotate(XY(event->x/get_width(),event->y/get_height()),false,false);
  }else if( (event->state & GDK_BUTTON1_MASK) && b_zoom_frame){
    cursor(CursorZoomW);
    d.x=event->x;
    d.y=event->y;
  }
  queue_draw();
  return true;
}

bool Gtk::ArcballWidget::on_key_press_event(GdkEventKey* event){
	switch(event->keyval){
    case GDK_KP_Home:
    case GDK_KP_7:
      reset(event->state&GDK_SHIFT_MASK);
      redraw();
      return 1;
    case GDK_KP_Add:
    case GDK_plus:
      //ZoomIn();
      transform./*post_*/scale(XYZ(1.1,1.1,1.1));
      redraw();
      return 1;
    case GDK_KP_Subtract:
    case GDK_minus:
      //ZoomOut();
      transform./*post_*/scale(XYZ(.9,.9,.9));
      redraw();
      return 1;
      //trenslate
    case GDK_KP_8:
      transform.translate(0, -Rmax * 0.1, 0.);
      redraw();
      return 1;
    case GDK_KP_2:
      transform.translate(0, Rmax * 0.1, 0.);
      redraw();
      return 1;
    case GDK_KP_4:
      transform.translate(Rmax * 0.1, 0, 0.);
      redraw();
      return 1;
    case GDK_KP_6:
      transform.translate(-Rmax * 0.1, 0, 0.);
      redraw();
      return 1;
	}
  return Gtk::GL::DrawingArea::on_key_press_event(event);
}
bool Gtk::ArcballWidget::on_scroll_event(GdkEventScroll* event){
  if(GDK_SCROLL_UP == event->direction ||  GDK_SCROLL_LEFT ==  event->direction)transform.scale(XYZ(.9,.9,.9));
  else transform.scale(XYZ(1.1,1.1,1.1));
  // GDK_SCROLL_DOWN GDK_SCROLL_RIGHT
  queue_draw();
  return true;
}

#ifndef _WIN32

#include <pangomm.h>
#include <pango/pangoft2.h>

static Glib::RefPtr<Pango::Context> ft2_context;

static void
gl_pango_ft2_render_layout (Glib::RefPtr<Pango::Layout> layout)
{
  Pango::Rectangle unu,logical_rect;
  FT_Bitmap bitmap;
  GLvoid *pixels;
  guint32 *p;
  GLfloat color[4];
  guint32 rgb;
  GLfloat a;
  guint8 *row, *row_end;
  int i;

  layout->get_extents(unu,logical_rect);
  if (logical_rect.get_width() == 0 || logical_rect.get_height() == 0)
    return;

  bitmap.rows = PANGO_PIXELS (logical_rect.get_height());
  bitmap.width = PANGO_PIXELS (logical_rect.get_width());
  bitmap.pitch = bitmap.width;
  bitmap.buffer = (unsigned char*)g_malloc (bitmap.rows * bitmap.width);
  bitmap.num_grays = 256;
  bitmap.pixel_mode = ft_pixel_mode_grays;

  memset (bitmap.buffer, 0, bitmap.rows * bitmap.width);
  pango_ft2_render_layout (&bitmap, layout->gobj(),
                           PANGO_PIXELS (-logical_rect.get_x()), 0);

  pixels = g_malloc (bitmap.rows * bitmap.width * 4);
  p = (guint32 *) pixels;

  glGetFloatv (GL_CURRENT_COLOR, color);
#if !defined(GL_VERSION_1_2) && G_BYTE_ORDER == G_LITTLE_ENDIAN
  rgb =  ((guint32) (color[0] * 255.0))        |
        (((guint32) (color[1] * 255.0)) << 8)  |
        (((guint32) (color[2] * 255.0)) << 16);
#else
  rgb = (((guint32) (color[0] * 255.0)) << 24) |
        (((guint32) (color[1] * 255.0)) << 16) |
        (((guint32) (color[2] * 255.0)) << 8);
#endif
  a = color[3];

  row = bitmap.buffer + bitmap.rows * bitmap.width; /* past-the-end */
  row_end = bitmap.buffer;      /* beginning */

  if (a == 1.0)
    {
      do
        {
          row -= bitmap.width;
          for (i = 0; i < bitmap.width; i++)
#if !defined(GL_VERSION_1_2) && G_BYTE_ORDER == G_LITTLE_ENDIAN
            *p++ = rgb | (((guint32) row[i]) << 24);
#else
            *p++ = rgb | ((guint32) row[i]);
#endif
        }
      while (row != row_end);
    }
  else
    {
      do
        {
          row -= bitmap.width;
          for (i = 0; i < bitmap.width; i++)
#if !defined(GL_VERSION_1_2) && G_BYTE_ORDER == G_LITTLE_ENDIAN
            *p++ = rgb | (((guint32) (a * row[i])) << 24);
#else
            *p++ = rgb | ((guint32) (a * row[i]));
#endif
        }
      while (row != row_end);
    }

  glPixelStorei (GL_UNPACK_ALIGNMENT, 4);

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if !defined(GL_VERSION_1_2)
  glDrawPixels (bitmap.width, bitmap.rows,
                GL_RGBA, GL_UNSIGNED_BYTE,
                pixels);
#else
  glDrawPixels (bitmap.width, bitmap.rows,
                GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
                pixels);
#endif

  glDisable (GL_BLEND);

  g_free (bitmap.buffer);
  g_free (pixels);
}

void Gtk::ArcballWidget::stext(int size, GLfloat x, GLfloat y, GLfloat z,const char *str)const{
  Glib::RefPtr<Pango::Context> widget_context = const_cast<Gtk::ArcballWidget*>(this)->get_pango_context(); 
  
  Pango::FontDescription font_desc=widget_context->get_font_description();

  Pango::Rectangle un,logical_rect;
  GLfloat text_w, text_h;
  GLfloat tangent_h;
  /* Font */
  font_desc.set_size(size * PANGO_SCALE);
  if(!ft2_context)ft2_context=Glib::wrap(pango_ft2_get_context(72,72));
  ft2_context->set_font_description(font_desc);

  /* Text layout */
  Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(ft2_context);
  layout->set_width(PANGO_SCALE * get_width());
  layout->set_alignment(Pango::ALIGN_CENTER);
  layout->set_text(str);

  /*** OpenGL BEGIN ***/
  layout->get_extents(un,logical_rect);
  text_w = PANGO_PIXELS (logical_rect.get_width());
  text_h = PANGO_PIXELS (logical_rect.get_height());

  tangent_h = 3 * tan (20 * G_PI / 180.0);
  tangent_h /= get_height();
  glRasterPos3f (x,y,z);

  /* Render text */
  gl_pango_ft2_render_layout (layout);

}

#else
void Gtk::ArcballWidget::stext(int size,GLfloat x, GLfloat y, GLfloat z,const char *txt)const{
}
#endif

