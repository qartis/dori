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

#include "FlGlArcballWindow.h"
#include "FL/Fl_Double_Window.H"
#include "FL/Fl_Check_Button.H"
#include "FL/Fl_Choice.H"

#include "draw_cube.h"

//make derived class
class testarc : public FlGlArcballWindow{
  public:
    testarc(int X,int Y,int W,int H,const char*L=0):FlGlArcballWindow(X,Y,W,H,L),show_axis(true),show_3d_ort(true),show_scale(true){}
    //rewrite draw method
    virtual void draw(){
      reshape();
      if(!valid()){
      static GLfloat light_diffuse[] = {1.0, 0.0, 0.0, 1.0};
      static GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};
      glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
      glLightfv(GL_LIGHT0, GL_POSITION, light_position);
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);
      }
      valid(1);
      glClearColor (0.25,0.5,0.5, 1.0);
      glDepthFunc (GL_LESS);
      glEnable (GL_DEPTH_TEST);
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glPushMatrix();
      arcball_transform();//apply transformation

      if(show_axis)draw_axis();
      drawCube();
      if(show_3d_ort)draw_3d_orbit();
      if(show_scale)draw_zoom_scale();
      glPopMatrix();
      glFinish();
    }
    bool show_axis;
    bool show_3d_ort;
    bool show_scale;
    static void cb_axis(Fl_Check_Button*w,testarc*p){
      p->show_axis = w->value();
      p->redraw();
    }
    static void cb_3d(Fl_Check_Button*w,testarc*p){
      p->show_3d_ort = w->value();
      p->redraw();
    }
    static void cb_sc(Fl_Check_Button*w,testarc*p){
      p->show_scale = w->value();
      p->redraw();
    }
    static void cb_custom_view(Fl_Choice*w,testarc*p){
     int val = w->value();
     p->views3D((Views3D)(val+1));
    }
};


int main(int argc,char** argv){
  Fl_Double_Window win(640,480,"Multi Arcball example");
  Fl_Group tb(0,0,640,25);
  Fl_Check_Button btax(0,0,100,25,"Show Axes");
  Fl_Check_Button bt3d(102,0,125,25,"Show 3D orts");
  Fl_Check_Button btsc(230,0,100,25,"Show Scale");
  Fl_Choice ch3dv(400,0,125,25,"3D View");
  tb.resizable(NULL);
  ch3dv.add("Left|Right|Top|Bottom|Front|Back|LeftTopFront|RightTopFront|LeftBottomFront|RightBottomFront|LeftTopBack|RightTopBack|"
	    "LeftBottomBack|RightBottomBack");
  tb.end();
  win.add(tb);
  testarc arc(2,27,636,450);
  win.add(arc);
  win.resizable(arc);
  //
  btax.callback((Fl_Callback*)testarc::cb_axis,&arc);
  btax.value(1);
  bt3d.callback((Fl_Callback*)testarc::cb_3d,&arc);
  bt3d.value(1);
  btsc.callback((Fl_Callback*)testarc::cb_sc,&arc);
  btsc.value(1);
  ch3dv.callback((Fl_Callback*)testarc::cb_custom_view,&arc);
  ch3dv.value(0);
  win.show(argc,argv);
  return Fl::run();
}
