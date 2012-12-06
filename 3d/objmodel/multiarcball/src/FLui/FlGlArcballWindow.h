/* 
   Copyright 2004-2008 by the Yury Fedorchenko.
   
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

#ifndef _FL_GL_MARCBALL_WINDOW_H_
#define _FL_GL_MARCBALL_WINDOW_H_

#ifdef _MSC_VER
#ifdef FLMA_DLL
#  define FLMA_API __declspec(dllexport)
#else
#  define FLMA_API __declspec(dllimport)
#endif
#else
#  define FLMA_API
#endif


// Define macros for FLTK mouse buttons.
#define FL_LEFT_MOUSE 1
#define FL_MIDDLE_MOUSE 2
#define FL_RIGHT_MOUSE 3

#include "3dcontrols.h"
#include "basic_ball.h"
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>
#include <GL/glu.h>

/**
 * Class for a FLTK GL Arcball window, which handles arcball rotations by default
 */
class FLMA_API FlGlArcballWindow : public Fl_Gl_Window , public controls3d::basic_ball{
  void init();
public:
  virtual int view_width(){return w();}
  virtual int view_height(){return h();}
  
  FlGlArcballWindow(int w, int h, const char * l = NULL)
    : Fl_Gl_Window(w,h,l){init();}
  
  FlGlArcballWindow(int x, int y, int w, int h, const char * l = NULL)
    : Fl_Gl_Window(x,y,w,h,l){init();}
  
  virtual int handle(int event);
  void draw_overlay();
protected:
  virtual void stext(int size, GLfloat x, GLfloat y, GLfloat z,const char *str)const;
public:
  virtual void cursor(const Cursors3D cur=CursorDefault);
};

#endif
