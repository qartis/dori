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

#include "GlArcballWindow.h"
#include "fltk/run.h"

#include "draw_cube.h"

class testarc : public fltk::ext::GlArcballWindow{
  public:
    testarc(int W,int H,const char*L=0):GlArcballWindow(W,H,L){}
    void draw(){
      reshape ();
      if(!valid()){
      static GLfloat light_diffuse[] = {1.0, 0.0, 0.0, 1.0};
      static GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};
      glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
      glLightfv(GL_LIGHT0, GL_POSITION, light_position);
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);
      }
      valid (1);
      glClearColor (0.25,0.5,0.5, 1.0);
      glDepthFunc (GL_LESS);
      glEnable (GL_DEPTH_TEST);

      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      arcball_transform();
      glPushMatrix();

      draw_axis();
      drawCube();
      glPopMatrix();
      draw_3d_orbit();
      glFinish();
    }
};


int main(int argc,char** argv){
  testarc arc(320,240,"MA test fltk2");
  arc.resizable(arc);
  arc.show(argc,argv);
  return fltk::run();
}
