/* 
   Copyright 2007-2008 by the Yury Fedorchenko.
   
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

#ifndef _GTKMM_ARCBALL_WINDOW_H_
#define _GTKMM_ARCBALL_WINDOW_H_

#ifdef _MSC_VER
#ifdef GTKMA_DLL
#  define GTKMA_API __declspec(dllexport)
#else
#  define GTKMA_API __declspec(dllimport)
#endif
#else
#  define GTKMA_API
#endif

#include <gtkmm-2.4/gtkmm.h>
#include <gtkglmm.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "3dcontrols.h"
#include "basic_ball.h"

namespace Gtk{
/**
 * 
 * 
 * Class for a FLTK GL Arcball window, which handles arcball rotations by default
 */
class GTKMA_API ArcballWidget : public Gtk::GL::DrawingArea , public controls3d::basic_ball{
public:
  virtual int view_width(){return get_width();}
  virtual int view_height(){return get_height();}

  void views3D(Views3D cv){
    basic_ball::views3D(cv);
    queue_draw();
  }

  ArcballWidget(const Glib::RefPtr<const Gdk::GL::Config>& config);
  
  // Derived classes can override this
  bool on_configure_event(GdkEventConfigure* event);

  //events
  virtual bool on_button_press_event (GdkEventButton* event);
  virtual bool on_button_release_event (GdkEventButton* event);
  virtual bool on_motion_notify_event (GdkEventMotion* event);
  //mouse wheel
  virtual bool on_scroll_event (GdkEventScroll* event);
  //
  bool on_key_press_event(GdkEventKey* event);
protected:
  virtual void stext(int size, GLfloat x, GLfloat y, GLfloat z,const char *str)const;
public:
  virtual void cursor(const Cursors3D cur=CursorDefault);
	
	void redraw(){queue_draw();}
};

};//Gtk
#endif
