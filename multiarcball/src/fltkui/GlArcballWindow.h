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

#ifdef _MSC_VER
#ifdef FL2MA_DLL
#  define FL2MA_API __declspec(dllexport)
#else
#  define FL2MA_API __declspec(dllimport)
#endif
#else
#  define FL2MA_API
#endif
#ifndef _GL_ARCBALL_WINDOW_H_
#define _GL_ARCBALL_WINDOW_H_


#include <fltk/events.h>
#include <fltk/GlWindow.h>
#include <fltk/gl.h>
#include <GL/glu.h>
#include "3dcontrols.h"
#include "basic_ball.h"

namespace fltk {
  namespace ext {
/**
 * Class for a FLTK GL Arcball window, which handles arcball rotations by default
 */
    class FL2MA_API GlArcballWindow:public GlWindow, public controls3d::basic_ball {
      void init();
    public:
      virtual int view_width() {
	return w();
      }
      virtual int view_height() {
	return h();
      }
      GlArcballWindow(int w, int h, const char *l = NULL)
        : GlWindow(w, h, l) { init();
      } 
      GlArcballWindow(int x, int y, int w, int h, const char *l = NULL)
        : GlWindow(x, y, w, h, l) { init();
      }
      virtual int handle(int event);
      void draw_overlay();
    protected:
      virtual void stext(int ,GLfloat x, GLfloat y, GLfloat z, const char *str) const;
    public:
      virtual void cursor(const Cursors3D cur=CursorDefault);
    };
  }//ext
}//fltk
#endif
