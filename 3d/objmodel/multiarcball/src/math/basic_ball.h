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

#ifndef _basic_ball_controll_
#define _basic_ball_controll_

#include "3dcontrols.h"
#include <GL/gl.h>
#include <GL/glu.h>


namespace controls3d{
/**
base class for all GUI widgets
*/
class MA_API basic_ball{
protected:
  bool b_zoom_frame;
  mvct::XY d,mouse;

  controls3d::ArcballCntrl arcball;    ///< The arcball controller
  controls3d::PanCntrl trcontrol;      ///< Translation controller
  controls3d::ZoomCntrl zoomcontrol;   ///< Zoom controller
  controls3d::DollyCntrl dollycontrol; ///< Dolly controller (10.0 scale)
  
  controls3d::Transformation transform;   ///< Combined transformation
  controls3d::Transformation transRotate; ///< Only rotation transformation
  controls3d::TransformsType currenttr;   ///< Current transformation

  // View/projection parameters
  enum ProjectionType { Orthographic=0, Perspective=1 };
  ProjectionType projtype;                          ///< Ortho/Perspective projection (Perspective not fully realized)
  
  /// For perspective projection
  double fovy;                                      //< Field-of-view in degrees in y
  double aspect_ratio;                              //< Ratio of width to height
  double near_cp;                                   //< Near clipping plane
  double far_cp;                                    //< Far clipping plane
  
  /// For orthographic projection
  double umin,umax,vmin,vmax;                       //< Viewing frustum
  
  double cenx, ceny, cenz;                          //< Center point for gluLookAt
  double eyex, eyey, eyez;                          //< Eye point for gluLookAt
  double upx, upy, upz;                             //< Up vector for gluLookAt
  double dist;                                      //< Dist from eye pt to center pt
  
  mvct::XYZ near_color_;
  mvct::XYZ far_color_;
  mvct::XYZ drag_color_;
  mvct::XYZ rim_color_;
  //
//  double Rmax;
  // get width and height of view port (gl window)
  virtual int view_width()=0;
  virtual int view_height()=0;
public:
  double Rmax;
  basic_ball();
  virtual ~basic_ball(){};

  void reshape();
  /**Prescribed 3D views  
  */
  enum Views3D{Nochange=0,Left,Right,Top,Bottom,Front,Back,
  LeftTopFront,RightTopFront,LeftBottomFront,RightBottomFront,
  LeftTopBack,RightTopBack,LeftBottomBack,RightBottomBack}; 
  
  virtual void views3D(Views3D cv);///<set prescribed 3D views  

  void draw_zoom_frame(); ///< draw frame in region zoom mode
  void draw_axis();   ///< draw axis 
  void draw_zoom_scale();   ///< draw axis 
  void draw_3d_orbit(); ///< draw 3D orts

  virtual void handle_rotate(mvct::XY mouse,bool push,bool release);//< @par mouse - XY mouse coords dividing by widget W/H
  virtual void handle_pan(mvct::XY mouse,bool push,bool release);//< @par mouse - XY mouse coords dividing by widget W/H
  virtual void handle_zoom(mvct::XY mouse,bool push,bool release);//< @par mouse - XY mouse coords dividing by widget W/H
  virtual void handle_dolly(mvct::XY mouse,bool push,bool release);//< @par mouse - XY mouse coords dividing by widget W/H  

  //transformations 
  void arcball_transformRotate()const; //< Apply the arcball transformation nozoom no pan
  void arcball_transform()const;   //< Apply the arcball transformation
  void reset(bool reset_rotation=true) { //< Reset transformation @par reset_rotation - if false only zoom to initial value
    if (reset_rotation) {
      transRotate.reset();
    }
    transform = transRotate;
  }
  void zoom_in(){transform.scale(1.1, 1.1, 1.1);}
  void zoom_out(){transform.scale(.9, .9, .9);}
/*  void arcball_draw(void);                           // Draw the arcball
  void drawConstraints (void) const;
  void drawDragArc (void);*/
  //TEXTOUT
  void text(GLfloat x, GLfloat y, GLfloat z,const char *format, ...)const;//default size is 12
  void text(int size, GLfloat x, GLfloat y, GLfloat z,const char *format, ...)const;//
protected:
  virtual void stext(int size,GLfloat x, GLfloat y, GLfloat z,const char *str)const=0;
public:
  //Cursor Types
  enum Cursors3D{ CursorNone=0,CursorDefault=1,CursorMove,CursorZoom,CursorRotate,CursorRotateX,
    CursorRotateY,CursorRotateZ,CursorZoomW};
  virtual void cursor(){};
};

};

#endif //_basic_ball_controll_

