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
#include "vmmath.h"

#include "draw_cube.h"

class TestArc : public Gtk::ArcballWidget{
  bool b_axis;
  public:
  TestArc(const Glib::RefPtr<const Gdk::GL::Config>& config):Gtk::ArcballWidget(config),b_axis(true){
    set_size_request(640,480);
  };

  bool on_expose_event(GdkEventExpose* event){
    Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();
    // GL calls.
    // *** OpenGL BEGIN ***
    if (!glwindow->gl_begin(get_gl_context())) return false;
    glClearColor(0.25,0.5,0.5, 1.0);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    static GLfloat light_diffuse[] = {1.0, 0.0, 0.0, 1.0};
    static GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    arcball_transform();
    if(b_axis)draw_axis();
    //Gdk::GL::Drawable::draw_teapot(true,.5);
    drawCube();
    glPopMatrix();    
    draw_3d_orbit();
    draw_zoom_frame();//this call need for window zoom;
    // Swap buffers.
    if (glwindow->is_double_buffered()) glwindow->swap_buffers();
    else glFlush();
    glwindow->gl_end();
    // *** OpenGL END ***
    return true;
  }
};

int main(int argc,char** argv){
  Gtk::Main m(&argc,&argv);
  Gtk::GL::init(argc, argv);
  Glib::RefPtr<Gdk::GL::Config> glconfig;

  // Try double-buffered visual
  glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB    |
                                     Gdk::GL::MODE_DEPTH  |
                                     Gdk::GL::MODE_DOUBLE);
  if (!glconfig) {
      std::cerr << "*** Cannot find the double-buffered visual.\n"
                << "*** Trying single-buffered visual.\n";

      // Try single-buffered visual
      glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB   |
                                         Gdk::GL::MODE_DEPTH);
      if (!glconfig) {
          std::cerr << "*** Cannot find any OpenGL-capable visual.\n";
          return 0;
      }
  }
  Gtk::Window wnd;
  wnd.set_title("MA test GTKMM");
  TestArc arc(glconfig);
  wnd.add(arc);
  wnd.show_all_children();
  m.run(wnd);
  return 0;
}
